package main

type RootEntity struct {
	DownloadImg     DownloadImgEntity `json:"DownloadImg"`
	ImgSize         ImgSizeEntity     `json:"ImgSize"`
	LocalImg        LocalImgEntity    `json:"LocalImg"`
	MemberRateLimit int64             `json:"MemberRateLimit"`
}

type DownloadImgEntity struct {
	AllowMember bool `json:"Allow-Member"`
}

type ImgSizeEntity struct {
	MaxFileSize int64 `json:"maxFileSize"`
	MaxHeight   int64 `json:"maxHeight"`
	MaxWidth    int64 `json:"maxWidth"`
}

type LocalImgEntity struct {
	AllowMember bool `json:"Allow-Member"`
}
