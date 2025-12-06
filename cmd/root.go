package cmd

import (
	"os"
	"github.com/spf13/cobra"
)

var rootCmd = &cobra.Command{
	Use:   "symbiote",
	Short: "Symbiote: Environmental Keyed Payload Generator",
	Long:  `A Red Team tool to generate and serve hardware-locked payloads.`,
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