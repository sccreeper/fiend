package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"log"
	"time"

	"fyne.io/fyne/v2"
	"fyne.io/fyne/v2/app"
	"fyne.io/fyne/v2/container"
	"fyne.io/fyne/v2/widget"
	"github.com/gordonklaus/portaudio"
	"github.com/urfave/cli/v2"
)

var speakButton *widget.Button
var responseText *widget.Label
var statusText *widget.Label
var speaking bool

var audioDeviceToBeUsed *portaudio.DeviceInfo
var stream *portaudio.Stream
var buffer *bytes.Buffer

const stringStartSpeaking string = "Start speaking"
const stringStopSpeaking string = "Stop speaking"
const stringProcessing string = "Processing"

func recordAudio() error {

	buffer = bytes.NewBuffer([]byte{})

	var err error
	stream, err = portaudio.OpenStream(portaudio.StreamParameters{
		Input: portaudio.StreamDeviceParameters{
			Device:   audioDeviceToBeUsed,
			Channels: 1,
		},
		SampleRate:      float64(DefaultSampleRate),
		FramesPerBuffer: int(DefaultFramesPerBuffer),
	}, func(in []int16) {
		binary.Write(buffer, binary.LittleEndian, in)
	})
	if err != nil {
		return err
	}

	err = stream.Start()
	if err != nil {
		return err
	}

	return nil

}

func stopRecordingAudio() error {

	time.Sleep(100 * time.Millisecond)
	stream.Stop()
	stream.Close()

	log.Println("Processing audio")

	resp, err := getLlmResponse(buffer)
	if err != nil {
		return err
	}

	responseText.SetText(
		fmt.Sprintf(
			"You: %s\nFiend:\n%s",
			resp.UserMessage,
			resp.LlmResponse,
		),
	)

	return nil

}

func startUi(ctx *cli.Context) error {

	var err error

	audioDeviceToBeUsed, err = portaudio.DefaultInputDevice()
	if err != nil {
		return err
	}

	a := app.New()
	w := a.NewWindow("Fiend")

	speakButton = widget.NewButton(stringStartSpeaking, func() {
		if speaking {
			speakButton.SetText(stringProcessing)
			speakButton.Disable()
			defer speakButton.Enable()
			defer speakButton.SetText(stringStartSpeaking)

			speaking = !speaking

			log.Println("Stopped speaking")
			stopRecordingAudio()
		} else {
			speakButton.SetText(stringStopSpeaking)
			speaking = !speaking
			log.Println("Started speaking")
			recordAudio()
		}
	})

	statusText = widget.NewLabelWithStyle("", fyne.TextAlignLeading, fyne.TextStyle{Italic: true})
	responseText = widget.NewLabel("")
	responseText.Wrapping = fyne.TextWrapBreak

	w.SetContent(
		container.NewPadded(
			container.NewVBox(
				widget.NewLabelWithStyle("Talk to fiend", fyne.TextAlignCenter, fyne.TextStyle{}),
				speakButton,
				statusText,
				responseText,
			),
		),
	)

	w.ShowAndRun()

	return nil

}
