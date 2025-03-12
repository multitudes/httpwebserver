# tunnels


you can create a connection between your local machine and a remote server using Caddy's tunnel feature. This feature allows you to securely expose your local services to the internet without the need to open ports on your firewall or router. You can use tunnels to access services running on your local machine from anywhere in the world.

Also you can use ngrok or cloudflare to create tunnels.

Download the Cloudflare Tunnels CLI from the Cloudflare website and install it on your local machine. You can then use the following command to create a tunnel:

```bash	
cloudflared tunnel --url https://localhost:8080
```
and it will spit out a URL where anyone can access your local service.