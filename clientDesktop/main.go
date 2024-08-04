package main

import (
	"os"

	"github.com/gordonklaus/portaudio"
	"github.com/urfave/cli/v2"
)

func main() {

	cmd := &cli.App{
		Name:           "fiend",
		Usage:          "Talk to your best fiend",
		DefaultCommand: "ui",
		Commands: []*cli.Command{
			{
				Name:   "cli",
				Usage:  "use the cli interface",
				Action: startCli,
			},
			{
				Name:   "ui",
				Usage:  "use the ui",
				Action: startUi,
			},
		},
	}

	// Init Audio

	portaudio.Initialize()
	defer portaudio.Terminate()

	// Start app

	if err := cmd.Run(os.Args); err != nil {
		panic(err)
	}

}
