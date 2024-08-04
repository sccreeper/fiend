package main

import (
	"bytes"
	"encoding/json"
	"io"
	"net/http"
)

const DefaultSampleRate int64 = 8000
const DefaultFramesPerBuffer int64 = 512
const DefaultChannels int64 = 1

type FiendResponse struct {
	UserMessage string `json:"userMessage"`
	LlmResponse string `json:"llmResponse"`
}

func getLlmResponse(buffer *bytes.Buffer) (FiendResponse, error) {

	res, err := http.Post("http://localhost:8787/api/askQuestion", "application/octet-stream", buffer)
	if err != nil {
		panic(err)
	}
	defer res.Body.Close()

	var responseJson FiendResponse

	bodyBytes, err := io.ReadAll(res.Body)
	if err != nil {
		panic(err)
	}

	err = json.Unmarshal(bodyBytes, &responseJson)
	if err != nil {
		panic(err)
	}

	return responseJson, nil

}
