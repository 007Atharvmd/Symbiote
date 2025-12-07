package cmd

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"
)

// 1. Define the Banner Function (Public so other commands can use it)
func PrintBanner() {
	// ANSI Color Codes
	Red := "\033[31m"
	Reset := "\033[0m"

	// The Symbiote ASCII Art
	banner := `
   (                      )
   |\    _,--------._    / |
   | \_ /            \ _/  |
   |  ./  .   .   .   \.   |
   |  |   |   |   |   |    |
   |  |   |   |   |   |    |
    \  \  |   |   |  /    /
     \  \ \___|___/ /    /
      \  \_________/    /
       \               /
        \_____________/
      S  Y  M  B  I  O  T  E
`
	fmt.Println(Red + banner + Reset)
}

var rootCmd = &cobra.Command{
	Use:   "symbiote",
	Short: "Symbiote: Environmental Keyed Payload Generator",
	Long:  `A Red Team tool to generate and serve hardware-locked payloads.`,
	// 2. Add Run logic to show banner when user just types "./symbiote"
	Run: func(cmd *cobra.Command, args []string) {
		PrintBanner()
		cmd.Help() // Show the help menu after the banner
	},
}

func Execute() {
	err := rootCmd.Execute()
	if err != nil {
		os.Exit(1)
	}
}

func init() {
	// Global flags can go here
}