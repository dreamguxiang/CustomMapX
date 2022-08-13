package main

import (
	"encoding/binary"
	"encoding/json"
	"image"
	"image/jpeg"
	"image/png"
	"net/url"
	"os"
	"strconv"
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
		ret.len = C.longlong(-2)
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

func isURL(urls string) bool {
	_, err := url.ParseRequestURI(urls)
	if err != nil {
		return false
	}
	return true
}

func ReadConfig() (int64, int64, int64) {
	data, err := ioutil.ReadFile("plugins\\CustomMapX\\config.json")
	if err != nil {
		return 0, 0, 0
	}
	var root RootEntity
	err = json.Unmarshal(data, &root)
	if err != nil {
		return 0, 0, 0
	}
	return root.ImgSize.MaxWidth, root.ImgSize.MaxHeight, root.ImgSize.MaxFileSize
}

//export getUrlPngData
func getUrlPngData(url string) C.CgoSlice {
	image.RegisterFormat("png", "png", png.Decode, png.DecodeConfig)
	image.RegisterFormat("jpeg", "jpeg", jpeg.Decode, jpeg.DecodeConfig)
	if isURL(url) {
		header, err := http.Head(url)
		if err != nil {
			var ret C.CgoSlice
			ret.data = nil
			ret.len = C.longlong(-4)
			ret.cap = C.longlong(0)
			return ret
		}
		maxw, maxh, maxsize := ReadConfig()
		length := float64(header.ContentLength) / 1024 / 1024
		if length != 0 && length <= float64(maxsize) {
			resp, err := http.Get(url)
			defer resp.Body.Close()
			if err != nil {
				var ret C.CgoSlice
				ret.data = nil
				ret.len = C.longlong(-1)
				ret.cap = C.longlong(0)
				return ret
			}
			allbody, err := ioutil.ReadAll(resp.Body)
			if err != nil {
				var ret C.CgoSlice
				ret.data = nil
				ret.len = C.longlong(-1)
				ret.cap = C.longlong(0)
				return ret
			}
			imgtype := GetImageType(ioutil.NopCloser(bytes.NewReader(allbody)))
			if imgtype != "" {
				if err != nil {
					var ret C.CgoSlice
					ret.data = nil
					ret.len = C.longlong(-3)
					ret.cap = C.longlong(0)
					return ret
				}
				img, _, err := image.Decode(ioutil.NopCloser(bytes.NewReader(allbody)))
				if err != nil {
					var ret C.CgoSlice
					ret.data = nil
					ret.len = C.longlong(-5)
					ret.cap = C.longlong(0)
					return ret
				}
				w, h := img.Bounds().Dx(), img.Bounds().Dy()
				if int64(w) > maxw || int64(h) > maxh {
					var ret C.CgoSlice
					ret.data = nil
					ret.len = C.longlong(-2)
					ret.cap = C.longlong(0)
					return ret
				}
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
		}
	}
	var ret C.CgoSlice
	ret.data = nil
	ret.len = C.longlong(-1)
	ret.cap = C.longlong(0)
	return ret
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
