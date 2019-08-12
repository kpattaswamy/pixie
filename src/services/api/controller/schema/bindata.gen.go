// Code generated by go-bindata.
// sources:
// schema.graphql
// DO NOT EDIT!

package schema

import (
	"bytes"
	"compress/gzip"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"
	"time"
)

func bindataRead(data []byte, name string) ([]byte, error) {
	gz, err := gzip.NewReader(bytes.NewBuffer(data))
	if err != nil {
		return nil, fmt.Errorf("Read %q: %v", name, err)
	}

	var buf bytes.Buffer
	_, err = io.Copy(&buf, gz)
	clErr := gz.Close()

	if err != nil {
		return nil, fmt.Errorf("Read %q: %v", name, err)
	}
	if clErr != nil {
		return nil, err
	}

	return buf.Bytes(), nil
}

type asset struct {
	bytes []byte
	info  os.FileInfo
}

type bindataFileInfo struct {
	name    string
	size    int64
	mode    os.FileMode
	modTime time.Time
}

func (fi bindataFileInfo) Name() string {
	return fi.name
}
func (fi bindataFileInfo) Size() int64 {
	return fi.size
}
func (fi bindataFileInfo) Mode() os.FileMode {
	return fi.mode
}
func (fi bindataFileInfo) ModTime() time.Time {
	return fi.modTime
}
func (fi bindataFileInfo) IsDir() bool {
	return false
}
func (fi bindataFileInfo) Sys() interface{} {
	return nil
}

var _schemaGraphql = []byte("\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\xff\x4c\x91\x41\xae\x9c\x30\x0c\x86\xf7\x39\xc5\x5f\xb1\xe8\xa6\x70\x00\x76\x95\xba\x61\xd7\xea\xbd\x39\x40\x48\x3c\xc4\x6a\x48\xf2\x12\xa7\x53\x54\xf5\xee\x4f\x04\x66\xc4\x0a\xc5\xfa\xfd\xf9\xc3\xee\xf0\xee\xb8\xe0\xce\x9e\x60\xa9\x98\xcc\x33\x15\x88\x23\x14\xe3\x68\xd5\xb8\xe7\xb8\xb6\xf7\xf7\x9f\x13\x0a\xe5\x3f\x6c\x68\x50\x9d\xea\x30\xc9\xd7\x82\x10\x05\x6c\x49\xfb\x6f\x98\xab\xe0\x41\x08\x44\x16\x12\xb1\xea\x50\xb5\xf7\x1b\x16\x0a\x94\xb5\x10\x64\x4b\x54\x70\x8f\xb9\xf1\xde\xb7\x44\x6f\x26\x73\x12\xdc\x26\xd5\xe1\xe1\x28\x40\x5e\x32\x5c\x50\x93\xd5\x42\x76\x38\x14\x8d\x0e\x98\x09\x36\x06\xc2\xbc\x21\xd7\x10\x38\x2c\xa3\xea\x80\x25\xeb\xe4\x3e\x7c\x7f\x28\xf7\x6d\xce\x41\x7e\xce\xee\xa5\x9c\x3f\x34\x9c\x61\xf4\x7d\xac\x92\xaa\x3c\xeb\x76\x90\xd2\x34\xd8\x38\x3c\xd8\xfb\x8b\xb8\x23\x9c\xe1\x9d\x7d\x08\x8a\xd3\x72\xe4\x66\x42\x62\xf3\x9b\x2c\x6a\xda\xd5\xf6\xf8\x6d\x1a\xd4\xb9\xdb\x0b\xbf\x75\x16\x14\x17\xab\xb7\xa0\xbf\x5c\x04\x1c\x8e\x75\xeb\x95\x60\x39\x93\x91\x98\x37\xe8\xeb\x11\x5e\xce\x7b\xfb\xa0\xd4\x79\x9a\x7f\x0a\xf8\xa8\x94\xb7\x11\xbf\xf6\x8f\xfa\xaf\x54\xf3\xbb\x15\xca\x53\xb8\xc7\x96\x60\x3b\x62\xfa\xf1\x45\x01\x41\xaf\x34\xe2\x4d\x32\x87\x65\x7f\xd3\xaa\xd9\x5f\x0b\x89\x8d\xd4\x7c\xc9\x3c\x81\x0d\xdf\x68\xb5\x50\x1e\x5f\x03\xf6\xc4\x67\x00\x00\x00\xff\xff\x2e\xf3\xa5\x22\x42\x02\x00\x00")

func schemaGraphqlBytes() ([]byte, error) {
	return bindataRead(
		_schemaGraphql,
		"schema.graphql",
	)
}

func schemaGraphql() (*asset, error) {
	bytes, err := schemaGraphqlBytes()
	if err != nil {
		return nil, err
	}

	info := bindataFileInfo{name: "schema.graphql", size: 578, mode: os.FileMode(436), modTime: time.Unix(1565111521, 0)}
	a := &asset{bytes: bytes, info: info}
	return a, nil
}

// Asset loads and returns the asset for the given name.
// It returns an error if the asset could not be found or
// could not be loaded.
func Asset(name string) ([]byte, error) {
	cannonicalName := strings.Replace(name, "\\", "/", -1)
	if f, ok := _bindata[cannonicalName]; ok {
		a, err := f()
		if err != nil {
			return nil, fmt.Errorf("Asset %s can't read by error: %v", name, err)
		}
		return a.bytes, nil
	}
	return nil, fmt.Errorf("Asset %s not found", name)
}

// MustAsset is like Asset but panics when Asset would return an error.
// It simplifies safe initialization of global variables.
func MustAsset(name string) []byte {
	a, err := Asset(name)
	if err != nil {
		panic("asset: Asset(" + name + "): " + err.Error())
	}

	return a
}

// AssetInfo loads and returns the asset info for the given name.
// It returns an error if the asset could not be found or
// could not be loaded.
func AssetInfo(name string) (os.FileInfo, error) {
	cannonicalName := strings.Replace(name, "\\", "/", -1)
	if f, ok := _bindata[cannonicalName]; ok {
		a, err := f()
		if err != nil {
			return nil, fmt.Errorf("AssetInfo %s can't read by error: %v", name, err)
		}
		return a.info, nil
	}
	return nil, fmt.Errorf("AssetInfo %s not found", name)
}

// AssetNames returns the names of the assets.
func AssetNames() []string {
	names := make([]string, 0, len(_bindata))
	for name := range _bindata {
		names = append(names, name)
	}
	return names
}

// _bindata is a table, holding each asset generator, mapped to its name.
var _bindata = map[string]func() (*asset, error){
	"schema.graphql": schemaGraphql,
}

// AssetDir returns the file names below a certain
// directory embedded in the file by go-bindata.
// For example if you run go-bindata on data/... and data contains the
// following hierarchy:
//     data/
//       foo.txt
//       img/
//         a.png
//         b.png
// then AssetDir("data") would return []string{"foo.txt", "img"}
// AssetDir("data/img") would return []string{"a.png", "b.png"}
// AssetDir("foo.txt") and AssetDir("notexist") would return an error
// AssetDir("") will return []string{"data"}.
func AssetDir(name string) ([]string, error) {
	node := _bintree
	if len(name) != 0 {
		cannonicalName := strings.Replace(name, "\\", "/", -1)
		pathList := strings.Split(cannonicalName, "/")
		for _, p := range pathList {
			node = node.Children[p]
			if node == nil {
				return nil, fmt.Errorf("Asset %s not found", name)
			}
		}
	}
	if node.Func != nil {
		return nil, fmt.Errorf("Asset %s not found", name)
	}
	rv := make([]string, 0, len(node.Children))
	for childName := range node.Children {
		rv = append(rv, childName)
	}
	return rv, nil
}

type bintree struct {
	Func     func() (*asset, error)
	Children map[string]*bintree
}
var _bintree = &bintree{nil, map[string]*bintree{
	"schema.graphql": &bintree{schemaGraphql, map[string]*bintree{}},
}}

// RestoreAsset restores an asset under the given directory
func RestoreAsset(dir, name string) error {
	data, err := Asset(name)
	if err != nil {
		return err
	}
	info, err := AssetInfo(name)
	if err != nil {
		return err
	}
	err = os.MkdirAll(_filePath(dir, filepath.Dir(name)), os.FileMode(0755))
	if err != nil {
		return err
	}
	err = ioutil.WriteFile(_filePath(dir, name), data, info.Mode())
	if err != nil {
		return err
	}
	err = os.Chtimes(_filePath(dir, name), info.ModTime(), info.ModTime())
	if err != nil {
		return err
	}
	return nil
}

// RestoreAssets restores an asset under the given directory recursively
func RestoreAssets(dir, name string) error {
	children, err := AssetDir(name)
	// File
	if err != nil {
		return RestoreAsset(dir, name)
	}
	// Dir
	for _, child := range children {
		err = RestoreAssets(dir, filepath.Join(name, child))
		if err != nil {
			return err
		}
	}
	return nil
}

func _filePath(dir, name string) string {
	cannonicalName := strings.Replace(name, "\\", "/", -1)
	return filepath.Join(append([]string{dir}, strings.Split(cannonicalName, "/")...)...)
}

