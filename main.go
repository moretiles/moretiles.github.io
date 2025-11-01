package main

import (
	//"fmt"
	//"bytes"
	"embed"
	//"encoding/json"
	"html/template"
	"log"
	//"path/filepath"
	"encoding/json"
	"fmt"
	"os"
	"strings"
)

// Assemble required functions to properly render template
func funcMap(tmpl **template.Template) (template.FuncMap, error) {
	// stores functions that will be used in rendering template
	var funcs template.FuncMap = map[string]interface{}{}

	/*
	 * insert preformatted text file located under pre/
	 * All files in pre/ are recursively loaded into the global variable root
	 */
	funcs["pre"] = func(filename string) (string, error) {
		fullpath := "pre" + "/" + filename
		byteString, err := root.ReadFile(fullpath)
		if err != nil {
			log.Fatal(err)
		}

		return string(byteString), nil
	}

	// Return json file that is an array of objects as a [](map[string]string)
	funcs["objects"] = func(filename string) ([](map[string]string), error) {
		var slice [](map[string]string) = make([](map[string]string), 99)
		fullpath := "json" + "/" + filename
		byteString, err := root.ReadFile(fullpath)
		if err != nil {
			log.Fatal(err)
		}
		err = json.Unmarshal(byteString, &slice)
		if err != nil {
			log.Fatal(err)
		}

		return slice, nil
	}

	return funcs, nil
}

// Collect a slice of all file paths (no directories) under the base path in files
func collectAll(files *embed.FS, base string) ([]string, error) {
	var paths []string
	dir, err := root.ReadDir(base)
	if err != nil {
		return nil, err
	}

	for _, file := range dir {
		fullName := base + "/" + file.Name()
		if file.IsDir() {
			children, err := collectAll(files, fullName)
			if err != nil {
				log.Fatal(err)
			}
			paths = append(paths, children...)
		} else {
			//fmt.Printf("%v/%v\n", base, file.Name())
			paths = append(paths, fullName)
		}
	}

	return paths, nil
}

// Treating each file specified as a template execute it writing the output under docs/
func executeAll(tmpl *template.Template, files []string) error {
	for _, fullpath := range files {
		// Determine filename
		splitSlice := strings.Split(fullpath, ".")
		splitSlice = splitSlice[:len(splitSlice)-1]
		filename := strings.Join(splitSlice, ".")

		// Determine templateName
		splitSlice = strings.Split(filename, "/")
		splitSlice = splitSlice[1:]
		templateName := strings.Join(splitSlice, "/")
		templateName = "/" + templateName

		// Determine outputPath
		splitSlice = strings.Split(filename, "/")
		splitSlice[0] = "docs"
		outputPath := strings.Join(splitSlice, "/")

		// Write data
		openFile, err := os.OpenFile(outputPath, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0644)
		if err != nil {
			log.Fatal(err)
		}
		defer openFile.Close()
		err = tmpl.ExecuteTemplate(openFile, templateName, nil)
		if err != nil {
			log.Fatal(err)
		}

		// Debug statement
		fmt.Println(outputPath)
	}

	return nil
}

/*
 * Data structure used to load all contents of html/, include/, pre/, and json recursively.
 * We can find files using their paths or by iterating over root directories.
 */
//go:embed html/* include/* pre/* json/*
var root embed.FS

func main() {
	// Load html files (html/), templates (include/)
	include, err := collectAll(&root, "include")
	if err != nil {
		log.Fatal(err)
	}
	html, err := collectAll(&root, "html")
	if err != nil {
		log.Fatal(err)
	}

	// Load functions required to process templates
	tmpl := template.New("")
	funcs, err := funcMap(&tmpl)
	if err != nil {
		log.Fatal(err)
	}
	tmpl = tmpl.Funcs(funcs)
	if err != nil {
		log.Fatal(err)
	}

	/*
	 * Load templates themselves
	 * The templates in include/ are consumed by the html templates in html/
	 */
	tmpl, err = tmpl.ParseFS(root, include...)
	if err != nil {
		log.Fatal(err)
	}
	tmpl, err = tmpl.ParseFS(root, html...)
	if err != nil {
		log.Fatal(err)
	}

	// Write templates to docs (output directory)
	executeAll(tmpl, html)
}
