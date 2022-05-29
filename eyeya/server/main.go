package main

import (
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"image"
	"image/jpeg"
	"io"
	"log"
	"net"
	"net/http"
	"sync"
	"time"

	_ "embed"

	"github.com/gin-gonic/gin"
)

const (
	PART_BOUNDARY       = "123456789000000000000987654321"
	STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" + PART_BOUNDARY
	STREAM_BOUNDARY     = "\r\n--" + PART_BOUNDARY + "\r\n"
	STREAM_PART         = "Content-Type: image/jpeg\r\n\r\n"
)

const (
	CamOnline  = "online"
	CamOffline = "offline"
)

//go:embed ace.html
var homePage []byte

var (
	webAddr    string
	brokerAddr string
	passwd     string
	deviceID   string
	debug      bool
	help       bool
)

func init() {
	flag.StringVar(&webAddr, "web-addr", ":4399", "serve web frontend")
	flag.StringVar(&brokerAddr, "broker-addr", ":9999", "serve iot device")
	flag.StringVar(&passwd, "passwd", "moyu@123", "passwd to access the web ui")
	flag.StringVar(&deviceID, "device-id", "ojbk", "camera device id to authencate")
	flag.BoolVar(&debug, "debug", true, "debug mode")
	flag.BoolVar(&help, "h", false, "print help info")
}

func main() {
	flag.Parse()
	if help {
		flag.Usage()
		return
	}

	e := NewEyeYa(brokerAddr, webAddr)
	e.ServeToDie()
}

type EyeYa struct {
	ch          chan byte
	streamOn    bool
	frameCh     chan *Frame
	broker, web net.Listener
	framePool   sync.Pool
	status      string
}

func NewEyeYa(brokerAddr, webAddr string) *EyeYa {
	e := new(EyeYa)
	var err error
	if e.broker, err = net.Listen("tcp", brokerAddr); err != nil {
		panic(err)
	}
	if e.web, err = net.Listen("tcp", webAddr); err != nil {
		panic(err)
	}

	e.ch = make(chan byte, 4)
	e.status = CamOffline
	e.streamOn = true
	e.frameCh = make(chan *Frame, 128)
	e.framePool = sync.Pool{
		New: func() interface{} {
			return &Frame{}
		},
	}
	return e
}

func (e *EyeYa) ServeToDie() {
	go e.runBrokerServer()

	e.runWebServer()
}

func (e *EyeYa) runWebServer() {
	r := gin.Default()

	r.GET("/", func(c *gin.Context) {
		c.Data(http.StatusOK, "text/html", homePage)
	})
	r.POST("/login", func(c *gin.Context) {
		pwd, ok := c.GetQuery("passwd")
		if !ok || pwd != passwd {
			c.Status(http.StatusUnauthorized)
			return
		}
		c.SetCookie("uid", passwd, 3600, "/", "*", false, false)
		c.Status(http.StatusOK)
	})
	r.GET("/device/status", func(c *gin.Context) {
		c.JSON(http.StatusOK, map[string]string{"status": e.status})
	})

	auth := func(c *gin.Context) {
		uid, _ := c.Cookie("uid")
		if uid != passwd {
			log.Println("no match passwd, unauthorized!")
			c.AbortWithStatus(http.StatusUnauthorized)
		}
		c.Next()
	}
	r.PUT("/pub/cam/:op", auth, func(c *gin.Context) {
		op := c.Param("op")
		switch op {
		case "on":
			e.pushCmd('O')
			e.streamOn = true
		case "off":
			e.pushCmd('C')
			e.streamOn = false
		}
	})
	r.PUT("/pub/motor/:cmd", auth, func(c *gin.Context) {
		cmd := c.Param("cmd")
		if len(cmd) < 1 {
			c.String(http.StatusBadRequest, "no cmd")
			return
		}
		for i := 0; i < len(cmd); i++ {
			e.pushCmd(cmd[i])
		}
		c.String(http.StatusOK, "ojbk")
	})
	r.GET("/sub/stream", auth, func(c *gin.Context) {
		c.Header("Content-Type", STREAM_CONTENT_TYPE)
		for e.streamOn {
			select {
			case <-c.Done():
				return
			case f := <-e.frameCh:
				c.Writer.Write([]byte(STREAM_BOUNDARY))
				c.Writer.Write([]byte(STREAM_PART))
				c.Writer.Write(f.Buf)
				e.framePool.Put(f)
			default:
			}
		}
	})

	panic(r.RunListener(e.web))
}

func (e *EyeYa) runBrokerServer() {
	for {
		conn, err := e.broker.Accept()
		if err != nil {
			log.Printf("[EyeYa] accept conn fail: %s\n", err.Error())
			continue
		}

		go e.handleConn(conn)
	}
}

func (e *EyeYa) pushCmd(c byte) {
	select {
	case e.ch <- c:
	default:
	}
}

func (e *EyeYa) handleConn(conn net.Conn) {
	connID := fmt.Sprintf("%s-%s", conn.RemoteAddr(), conn.LocalAddr())
	log.Printf("accept connection %s\n", connID)
	defer func() {
		log.Printf("close connection %s\n", connID)
		conn.Close()
		e.status = CamOffline
	}()

	if c, ok := conn.(*net.TCPConn); ok {
		log.Printf("[%s]set keepalive\n", connID)
		c.SetKeepAlive(true)
		c.SetKeepAlivePeriod(time.Second * 2)
	}

	// connect authorization
	devID := make([]byte, 4)
	if _, err := conn.Read(devID); err != nil {
		log.Printf("[%s]conn read err: %s", connID, err.Error())
		return
	}
	if !bytes.Equal(devID, []byte(deviceID)) {
		log.Printf("[%s]conn authorize fail, bad deviceID: %s", connID, string(devID))
		return
	}

	e.status = CamOnline

	quit := make(chan struct{})
	go func() {
		for {
			f := e.framePool.Get().(*Frame)
			if err := f.ReadOne(conn); err != nil {
				log.Printf("frame read one fail: %s\n", err.Error())
				break
			}
			log.Printf("frame read one: %d\n", f.Len)
			select {
			case e.frameCh <- f:
			default:
				e.framePool.Put(f)
			}
		}
		quit <- struct{}{}
	}()

	for {
		var c byte
		select {
		case c = <-e.ch:
		case <-quit:
			return
		}
		if _, err := conn.Write([]byte{c}); err != nil {
			log.Printf("[%s]got err: %s, close connection\n", connID, err.Error())
			break
		}
		log.Printf("[%s]sent command: %c\n", connID, c)
	}
}

// Frame video frame
type Frame struct {
	Len    uint32 // Length of the buffer in bytes
	Width  uint32 // Width of the buffer in pixels
	Height uint32 // Height of the buffer in pixels
	Buf    []byte // Pointer to the pixel data
	// pixformat_t format; //  Format of the pixel data
	// int64 timestamp;   // Timestamp since boot of the first DMA buffer of the frame
}

// ReadOne read one frame data once
func (f *Frame) ReadOne(r io.Reader) error {
	bs := make([]byte, 4)
	_, err := io.ReadFull(r, bs)
	if err != nil {
		return err
	}
	f.Len = binary.BigEndian.Uint32(bs)
	f.Buf = make([]byte, f.Len)
	_, err = io.ReadFull(r, f.Buf)
	return err
}

// Rotate the image with 90 degree
func (f *Frame) RotateWrite(w io.Writer) error {
	img, err := jpeg.Decode(bytes.NewReader(f.Buf))
	if err != nil {
		log.Println("image decode fail: ", err.Error())
		return err
	}
	r90 := image.NewRGBA(image.Rect(0, 0, img.Bounds().Dy(), img.Bounds().Dx()))
	for x := img.Bounds().Min.Y; x < img.Bounds().Max.Y; x++ {
		for y := img.Bounds().Max.X; y >= img.Bounds().Min.X; y-- {
			r90.Set(img.Bounds().Max.Y-x, y, img.At(y, x))
		}
	}

	return jpeg.Encode(w, r90, nil)
}
