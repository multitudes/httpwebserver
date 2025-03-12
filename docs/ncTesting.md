# nc testing



## Recognizing Multipart Requests

A multipart request is recognized by the `Content-Type: multipart/form-data` header, which includes a boundary parameter. The boundary parameter is used to separate the different parts of the request.

### Example of a Multipart Request

Here is a small example of a multipart request for testing:

```http
POST /upload HTTP/1.1
Host: example.com
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW

------WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Disposition: form-data; name="text"

This is a text field.
------WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Disposition: form-data; name="file"; filename="example.txt"
Content-Type: text/plain

This is the content of the file.
------WebKitFormBoundary7MA4YWxkTrZu0gW--
```

### Explanation:
1. **Headers**: The request starts with the usual HTTP headers, including `Content-Type: multipart/form-data` with a boundary parameter.
2. **Boundary**: The boundary string (`----WebKitFormBoundary7MA4YWxkTrZu0gW`) separates the different parts of the request.
3. **Parts**: Each part starts with a boundary, followed by headers for that part, and then the content.
   - **Text Field**: A text field named "text".
   - **File Field**: A file field named "file" with the filename "example.txt" and content type `text/plain`.

### Example Command for Testing

You can use `curl` to send a multipart request for testing:

```sh
curl -X POST http://localhost:4244/upload \
  -F "text=This is a text field." \
  -F "file=@example.txt"
```

### Explanation:
- **`-X POST`**: Specifies the HTTP method as POST.
- **`http://localhost:4244/upload`**: The URL to send the request to.
- **`-F "text=This is a text field."`**: Adds a text field named "text".
- **`-F "file=@example.txt"`**: Adds a file field named "file" with the content of the file `example.txt`.


with NC
```
echo -en "POST /upload HTTP/1.1\r\nHost: example.com\r\nContent-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n\r\n------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name=\"text\"\r\n\r\nThis is a text field.\r\n------WebKitFormBoundary7MA4YWxkTrZu0gW\r\nContent-Disposition: form-data; name=\"file\"; filename=\"example.txt\"\r\nContent-Type: text/plain\r\n\r\nThis is the content of the file.\r\n------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n" | nc localhost 4244
```


## ps and zombies
Yes, using `ps wufax` can help you identify zombie processes. Zombie processes are child processes that have completed execution but still have an entry in the process table. This happens when the parent process hasn't read the child's exit status, which is necessary for the operating system to remove the process entry.

In the output of `ps wufax`, zombie processes are typically indicated by a status of `Z` in the `STAT` column. The `STAT` column shows the current status of the process, and `Z` stands for "zombie."

Here's an example of what you might see:

```
USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root      1234  0.0  0.0      0     0 ?        Z    10:00   0:00 [zombie_process_name] <defunct>
```

In this example, the process with PID 1234 is a zombie process, as indicated by the `Z` in the `STAT` column and the `<defunct>` label in the `COMMAND` column.

If you identify zombie processes, you may need to investigate the parent process to ensure it is correctly handling child process termination. If the parent process is still running, it should be modified to properly wait for its child processes. If the parent process is no longer running, the zombie processes will be reaped by the `init` process (PID 1) eventually.



The command `ps afuxww | less -SR` is a combination of `ps` options and `less` options to display process information in a paginated and formatted manner. Here's a breakdown of what each part does:

### `ps afuxww`

- **`a`**: Show processes for all users. This includes processes not associated with a terminal.
- **`f`**: Display a full-format listing, which includes the process hierarchy (parent-child relationships).
- **`u`**: Display the process information in a user-oriented format, showing the user, CPU and memory usage, and start time.
- **`x`**: Include processes without a controlling terminal, such as daemon processes.
- **`ww`**: Use as many columns as necessary to display the information without truncating lines. This is similar to `w`, but more aggressive in preventing truncation.

### `| less -SR`

- **`|`**: This is a pipe, which takes the output of the `ps` command and passes it as input to the `less` command.
- **`less`**: A pager program that allows you to view the output one screen at a time.
- **`-S`**: Causes `less` to chop long lines rather than wrap them. This is useful for maintaining the alignment of columns in wide outputs.
- **`-R`**: Allows `less` to display raw control characters, which is useful if the output includes color or other formatting.

### Overall Effect

The command `ps afuxww | less -SR` will display a detailed, wide-format list of all processes on the system, including those without a terminal, in a paginated view. The `less` command allows you to scroll through the output, and the `-S` option ensures that long lines are chopped rather than wrapped, maintaining the readability of the columns. The `-R` option ensures that any color or formatting is preserved in the output.


also see kerrisk page 957 for reaping zombies