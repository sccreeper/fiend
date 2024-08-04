package main

import (
	"bytes"
	"encoding/json"
	"io"
	"net/http"
	"os"
)

const DefaultSampleRate int64 = 4000
const DefaultFramesPerBuffer int64 = 512
const DefaultChannels int64 = 1

type SpeechRecognitionResponse struct {
	Text string `json:"text"`
}

type FiendResponse struct {
	UserMessage string `json:"userMessage"`
	LlmResponse string `json:"llmResponse"`
}

func doSpeechRecognition(buffer *bytes.Buffer) (string, error) {

	res, err := http.Post("http://localhost:8787/api/speechToText", "application/octet-stream", buffer)
	if err != nil {
		panic(err)
	}
	defer res.Body.Close()

	var responseJson SpeechRecognitionResponse

	bodyBytes, err := io.ReadAll(res.Body)
	if err != nil {
		return "", nil
	}

	err = json.Unmarshal(bodyBytes, &responseJson)
	if err != nil {
		return "", nil
	}

	return responseJson.Text, nil

}

func getLlmResponse(query string) (FiendResponse, error) {

	resp, err := http.PostForm("http://localhost:8787/api/askQuestion", map[string][]string{"question": []string{query}})
	if err != nil {
		return FiendResponse{}, err
	}

	var responseJson FiendResponse

	bodyBytes, err := io.ReadAll(resp.Body)
	if err != nil {
		return FiendResponse{}, err
	}

	err = json.Unmarshal(bodyBytes, &responseJson)
	if err != nil {
		return FiendResponse{}, err
	}

	return responseJson, nil

}

func writeRaw(data []byte) error {

	bytesReader := bytes.NewReader(data)

	f, err := os.Create("raw.bin")
	if err != nil {
		return err
	}
	defer f.Close()

	io.Copy(f, bytesReader)

	return nil

}
