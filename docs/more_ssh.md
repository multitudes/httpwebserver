# connect via ssh

The `ssh` command is used to securely connect to a remote machine over a network. 

To connect to a local machine `ssh lb@localhost -p 2222`, connects `localhost` on port `2222` using the username `lb`.

This syntax `ssh myhost.example.com` is used to connect to a remote machine: `myhost.example.com` connects to the remote machine with the hostname `myhost.example.com` using the default SSH port `22` and the current user's username.

And using an Identity File (Private Key):
```sh
ssh -i /path/to/private_key username@myhost.example.com
```
