package main

import (
	"encoding/binary"
	"image"
	"image/jpeg"
	"image/png"
	"os"
	"strconv"
	"unsafe"
)

/*
#include <windows.h>
#include <stdlib.h>
typedef struct {
	void* data;
	long long len;
	long long cap;
} CgoSlice;
#cgo LDFLAGS: -Wl,--allow-multiple-definition
*/
import "C"
import (
	"bytes"
	"fmt"
	imgext "github.com/shamsher31/goimgext"
	_ "image/gif"
	"io"
	"io/ioutil"
	"net/http"
	"strings"
)

//export png2PixelArr
func png2PixelArr(paths string) C.CgoSlice {
	image.RegisterFormat("png", "png", png.Decode, png.DecodeConfig)
	image.RegisterFormat("jpeg", "jpeg", jpeg.Decode, jpeg.DecodeConfig)
	rr, err := os.Open(paths)
	if err != nil {
		var ret C.CgoSlice
		ret.data = nil
		ret.len = C.longlong(-1)
		ret.cap = C.longlong(0)
		return ret
	}
	defer rr.Close()
	img, _, err := image.Decode(rr)
	if err != nil {
		var ret C.CgoSlice
		ret.data = nil
		ret.len = C.longlong(-1)
		ret.cap = C.longlong(0)
		return ret
	}
	w, h := img.Bounds().Dx(), img.Bounds().Dy()
	data := make([]byte, 0, w*h*4)
	for y := 0; y < h; y++ {
		for x := 0; x < w; x++ {
			r, g, b, a := img.At(x, y).RGBA()
			data = append(data, byte(r>>8), byte(g>>8), byte(b>>8), byte(a>>8))
		}
	}
	out := []byte(strconv.Itoa(w))
	out = append(out, []byte{'\n'}...)
	out = append(out, strconv.Itoa(h)...)
	out = append(out, []byte{'\n'}...)
	out = append(out, data...)

	var ret C.CgoSlice
	ret.data = C.CBytes(out)
	ret.len = C.longlong(len(out))
	ret.cap = C.longlong(len(out))

	return ret
}

func IntToBytes(n int) []byte {
	x := int32(n)
	bytesBuffer := bytes.NewBuffer([]byte{})
	binary.Write(bytesBuffer, binary.BigEndian, x)
	return bytesBuffer.Bytes()
}

//export FreePointer
func FreePointer(p unsafe.Pointer) {
	C.free(p)
}

//export getPngWidth
func getPngWidth(paths string) int {
	r, err := os.Open(paths)
	if err != nil {
		return 0
	}
	defer r.Close()
	img, _, err := image.Decode(r)
	if err != nil {
		return 0
	}
	return img.Bounds().Dx()
}

//export getPngHeight
func getPngHeight(paths string) int {
	r, err := os.Open(paths)
	if err != nil {
		return 0
	}
	defer r.Close()
	img, _, err := image.Decode(r)
	if err != nil {
		return 0
	}
	return img.Bounds().Dy()
}

//export getUrlPngData
func getUrlPngData(url string) unsafe.Pointer {
	image.RegisterFormat("png", "png", png.Decode, png.DecodeConfig)
	image.RegisterFormat("jpeg", "jpeg", jpeg.Decode, jpeg.DecodeConfig)
	header, err := http.Head(url)
	if err != nil {
		return nil
	}
	length := float64(header.ContentLength) / 1024 / 1024
	if length != 0 && length <= 25 {
		resp, err := http.Get(url)
		defer resp.Body.Close()
		if err != nil {
			return nil
		}
		allbody, err := ioutil.ReadAll(resp.Body)
		if err != nil {
			return nil
		}
		imgtype := GetImageType(ioutil.NopCloser(bytes.NewReader(allbody)))
		if imgtype != "" {
			if err != nil {
				return nil
			}
			img, _, err := image.Decode(ioutil.NopCloser(bytes.NewReader(allbody)))
			if err != nil {
				fmt.Println(err.Error())
				return nil
			}
			w, h := img.Bounds().Dx(), img.Bounds().Dy()
			data := make([]byte, 0, w*h*4)
			for y := 0; y < h; y++ {
				for x := 0; x < w; x++ {
					r, g, b, a := img.At(x, y).RGBA()
					data = append(data, byte(r>>8), byte(g>>8), byte(b>>8), byte(a>>8))
				}
			}
			str := string(data)
			p := unsafe.Pointer(C.CString(str))
			return p
		}
	}
	return nil
}

func GetImageType(reader io.ReadCloser) string {
	buff := make([]byte, 512)
	_, err := reader.Read(buff)
	if err != nil {
		return ""
	}
	filetype := http.DetectContentType(buff)
	ext := imgext.Get()
	for i := 0; i < len(ext); i++ {
		if strings.Contains(ext[i], filetype[6:len(filetype)]) {
			return filetype
		}
	}
	return ""
}

func main() {

}
