package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"net/http"
	"os"
	"os/signal"
	"strconv"
	"time"

	"github.com/gordonklaus/portaudio"
)

const DefaultSampleRate int64 = 8000
const DefaultFramesPerBuffer int64 = 512
const DefaultChannels int64 = 1

func main() {

	var deviceToBeUsed *portaudio.DeviceInfo

	portaudio.Initialize()
	defer portaudio.Terminate()

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
			return
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
	}, func(in []int16) {
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

	bytesSlice := buffer.Bytes()
	bytesReader := bytes.NewReader(bytesSlice)

	fRaw, err := os.Create("raw.bin")
	if err != nil {
		panic(err)
	}
	defer fRaw.Close()
	io.Copy(fRaw, bytesReader)
	bytesReader.Seek(0, io.SeekStart)

	// Send data to server

	res, err := http.Post("http://localhost:8787/api/changePcmToWav", "application/octet-stream", buffer)
	if err != nil {
		panic(err)
	}
	defer res.Body.Close()

	f, err := os.Create("result.wav")
	if err != nil {
		panic(err)
	}
	defer f.Close()

	bytesWritten, err := io.Copy(f, res.Body)
	if err != nil {
		panic(err)
	}
	fmt.Printf("Written %d bytes", bytesWritten)

}
