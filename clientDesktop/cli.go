package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"os"
	"os/signal"
	"strconv"
	"time"

	"github.com/gordonklaus/portaudio"
	"github.com/urfave/cli/v2"
)

func startCli(ctx *cli.Context) error {

	portaudio.Initialize()

	var deviceToBeUsed *portaudio.DeviceInfo

	defaultInputDevice, err := portaudio.DefaultInputDevice()
	if err != nil {
		panic(err)
	}

	audioDevices, err := portaudio.Devices()
	if err != nil {
		panic(err)
	}

	for i, v := range audioDevices {

		if defaultInputDevice.Name == v.Name {
			fmt.Printf("%d: %s (DEFAULT)\n", i+1, v.Name)
		} else {
			fmt.Printf("%d: %s\n", i+1, v.Name)
		}
	}

	fmt.Println("Enter device number or press enter to start recording with default:")

	var deviceNumber string
	fmt.Scanln(&deviceNumber)

	if deviceNumber == "" {
		deviceToBeUsed = defaultInputDevice
	} else {

		no, err := strconv.Atoi(deviceNumber)
		if err != nil {
			panic(err)
		}

		if no <= 0 || no > len(audioDevices) {
			fmt.Println("number not in device list")
			return nil
		}

		deviceToBeUsed = audioDevices[no]

	}

	fmt.Println("Press Ctrl+C to stop recording")

	buffer := bytes.NewBuffer([]byte{})

	interruptSigChannel := make(chan os.Signal, 1)
	signal.Notify(interruptSigChannel, os.Interrupt)

	stream, err := portaudio.OpenStream(portaudio.StreamParameters{
		Input: portaudio.StreamDeviceParameters{
			Device:   deviceToBeUsed,
			Channels: 1,
		},
		SampleRate:      float64(DefaultSampleRate),
		FramesPerBuffer: int(DefaultFramesPerBuffer),
	}, func(in []uint8) {
		binary.Write(buffer, binary.LittleEndian, in)
	})

	if err != nil {
		panic(err)
	}

	err = stream.Start()
	if err != nil {
		panic(err)
	}

	var done bool

	for !done {

		select {
		case <-interruptSigChannel:
			time.Sleep(100 * time.Millisecond)
			done = true
		default:
		}

	}

	stream.Stop()
	stream.Close()

	fmt.Printf("Buffer size: %d\n", buffer.Len())

	speechRecognitionResponse, err := doSpeechRecognition(buffer)
	if err != nil {
		return err
	}

	// Send data to server

	fmt.Printf("You\n---\n")
	fmt.Println(speechRecognitionResponse)
	fmt.Printf("Fiend\n---\n")
	fmt.Printf("Thinking...")

	llmResponse, err := getLlmResponse(speechRecognitionResponse)
	if err != nil {
		return err
	}

	fmt.Printf("\r%s", llmResponse.LlmResponse)

	return nil
}
