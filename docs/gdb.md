## GBD - GNU Debugger
debug with 
```bash
gdb -tui ./miniRT
set args scene/earth.rt 
```
if the interface is scrambled ctrl-l to refresh the screen.  
```bash
b main
r
p *variable
n 
//when segfault where command shows the trace
where 
``` 