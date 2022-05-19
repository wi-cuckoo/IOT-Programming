package main

import (
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"image"
	"io"
	"log"
	"net"
	"net/http"
	"sync"
	"time"

	_ "embed"
	"image/jpeg"

	"github.com/gin-gonic/gin"
)

const (
	PART_BOUNDARY       = "123456789000000000000987654321"
	STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" + PART_BOUNDARY
	STREAM_BOUNDARY     = "\r\n--" + PART_BOUNDARY + "\r\n"
	STREAM_PART         = "Content-Type: image/jpeg\r\n\r\n"
)

//go:embed ace.html
var homePage []byte

var (
	webAddr    string
	brokerAddr string
	debug      bool
	help       bool
)

func init() {
	flag.StringVar(&webAddr, "web-addr", ":4399", "serve web frontend")
	flag.StringVar(&brokerAddr, "broker-addr", ":9999", "serve iot device")
	flag.BoolVar(&debug, "debug", true, "debug mode")
	flag.BoolVar(&help, "h", false, "print help info")
}

func main() {
	flag.Parse()
	if help {
		flag.Usage()
		return
	}

	e := NewEdged(webAddr, brokerAddr)
	e.Serve()
}

type Edged struct {
	ch        chan byte
	streamOn  bool
	frameCh   chan *Frame
	pub, sub  net.Listener
	framePool sync.Pool
}

func NewEdged(pubAddr, subAddr string) *Edged {
	e := new(Edged)
	var err error
	if e.pub, err = net.Listen("tcp", pubAddr); err != nil {
		panic(err)
	}
	if e.sub, err = net.Listen("tcp", subAddr); err != nil {
		panic(err)
	}
	e.ch = make(chan byte, 1)
	e.streamOn = true
	e.frameCh = make(chan *Frame, 128)
	e.framePool = sync.Pool{
		New: func() interface{} {
			return &Frame{}
		},
	}
	return e
}

func (e *Edged) Serve() {
	go e.serveSubStream()

	r := gin.Default()
	r.PUT("/pub/cam/:op", func(c *gin.Context) {
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
	r.PUT("/pub/motor/:cmd", func(c *gin.Context) {
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
	r.GET("/stream", func(c *gin.Context) {
		c.Header("Content-Type", STREAM_CONTENT_TYPE)
		for e.streamOn {
			select {
			case <-c.Done():
				return
			case f := <-e.frameCh:
				c.Writer.Write([]byte(STREAM_BOUNDARY))
				c.Writer.Write([]byte(STREAM_PART))
				f.RotateWrite(c.Writer)
				e.framePool.Put(f)
			}
		}
	})
	r.GET("/", func(c *gin.Context) {
		c.Data(http.StatusOK, "text/html", homePage)
	})

	panic(r.RunListener(e.pub))
}

func (e *Edged) serveSubStream() {
	for {
		conn, err := e.sub.Accept()
		if err != nil {
			log.Printf("[edged] accept conn fail: %s\n", err.Error())
			continue
		}
		go e.handleSubConn(conn)
	}
}

func (e *Edged) pushCmd(c byte) {
	select {
	case e.ch <- c:
	default:
	}
}

func (e *Edged) handleSubConn(conn net.Conn) {
	connID := fmt.Sprintf("%s-%s", conn.RemoteAddr(), conn.LocalAddr())
	log.Printf("accept connection %s\n", connID)
	defer func() {
		log.Printf("close connection %s\n", connID)
		conn.Close()
	}()

	if c, ok := conn.(*net.TCPConn); ok {
		log.Printf("[%s]set keepalive\n", connID)
		c.SetKeepAlive(true)
		c.SetKeepAlivePeriod(time.Second * 2)
	}

	// connect authorization
	passwd := make([]byte, 4)
	if _, err := conn.Read(passwd); err != nil {
		log.Printf("[%s]conn read err: %s", connID, err.Error())
		return
	}
	if !bytes.Equal(passwd, []byte("ojbk")) {
		log.Printf("[%s]conn authorize fail, bad passwd: %s", connID, string(passwd))
		return
	}

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
		log.Printf("[%s]write command: %c\n", connID, c)
		if _, err := conn.Write([]byte{c}); err != nil {
			log.Printf("[%s]got err: %s, close connection\n", connID, err.Error())
			break
		}
	}
}

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
