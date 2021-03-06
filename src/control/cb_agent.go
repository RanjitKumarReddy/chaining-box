package main

import (
  "control/cbox"
  "fmt"
  "os"
)

/* TODO: Make this configurable through the CLI */

func main(){
  if len(os.Args) < 5 {
    fmt.Printf("Usage: %s <name> <ingress-iface> <egress-iface> <objpath> [server-address]\n", os.Args[0])
    os.Exit(1)
  }

  name := os.Args[1]    /* Name to tell the manager */
  iiface := os.Args[2]   /* Ingress interface to use */
  eiface := os.Args[3]   /* Egress interface to use */
  objpath := os.Args[4] /* Path to BPF stages obj file */

  server_address := ":9000"
  if len(os.Args) == 6 {
    server_address = os.Args[5]
  }

  /* Allow not using ingress */
  if iiface == "-" {
    iiface = ""
  }

  /* Allow not using egress */
  if eiface == "-" {
    eiface = ""
  }

  /* Empty values for [e,i]iface will tell the agent that it should
   * not configure stages for that direction. */

  cba, err := cbox.NewCBAgent(name, iiface, eiface, objpath)
  if err != nil {
    fmt.Println(err)
    os.Exit(1)
  }

  err = cba.ManagerConnect(server_address)
  if err != nil {
    fmt.Println(err)
    os.Exit(1)
  }

  err = cba.ManagerListen()
  if err != nil {
    fmt.Println(err)
    os.Exit(1)
  }
}
