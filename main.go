package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"html/template"
	"log"
	"os"
	"path/filepath"
	"strings"
)

type data struct {
	SourceDir         string                         `json:"SourceDir"`
	OutputDir         string                         `json:"OutputDir"`
	BuildWants        []string                       `json:"BuildWants"`
	BuildTemplates    map[string][]string            `json:"BuildTemplates"`
	BuildDirWants     []string                       `json:"BuildDirWants"`
	BuildDirTemplates map[string]map[string][]string `json:"BuildDirTemplates"`
	FuncMap           template.FuncMap
}

func funcMap(tmpl **template.Template) (template.FuncMap, error) {
	// stores functions that will be used in rendering template
	var funcs template.FuncMap = map[string]interface{}{}

	// return the content of a file as a string
	funcs["fopen"] = func(filename string) string {
		bytes, err := os.ReadFile(filename)
		if err != nil {
			log.Fatal(err)
		}
		str := string(bytes)
		return str
	}

	// render the template named such and such a thing
	funcs["include"] = func(name string, data interface{}, times int) (template.HTML, error) {
		indent := "\r\n" + strings.Repeat(" ", times)
		buf := bytes.NewBuffer(nil)
		err := (*tmpl).ExecuteTemplate(buf, name, data)
		if err != nil {
			log.Fatal(err)
		}
		str := buf.String()
		split := strings.Split(strings.ReplaceAll(str, "\r\n", "\n"), "\n")
		str = strings.Join(split, indent)
		/*
			We trust that the evaluated template to be HTML.
			All child templates must ensure they escape text.
			Therefore when include consumes a template relevent content should already be escaped.
		*/
		return template.HTML(str), nil
	}
	return funcs, nil
}

func buildFiles(tmpl **template.Template, build data) {
	wants := build.BuildWants
	var context map[string]map[string]string

	for in, files := range build.BuildTemplates {
		split := strings.Split(in, `.`)
		if len(split) <= 1 || split[len(split)-1] != `tmpl` {
			log.Fatal(fmt.Errorf("Input template \"%s\" poorly named, must end in .tmpl", in))
		}
		out := strings.Join(split[0:len(split)-1], `.`)

		outFile, err := os.OpenFile(filepath.Join(build.OutputDir, out), os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0644)
		if err != nil {
			log.Fatal(err)
		}
		defer outFile.Close()

		contextFile := out + ".json"
		if _, err := os.Stat(contextFile); os.IsNotExist(err) {
			context = nil
		} else if err == nil {
			content, err := os.ReadFile(contextFile)
			if err != nil {
				log.Fatal(err)
			}
			err = json.Unmarshal(content, &context)
			if err != nil {
				log.Printf("%s\n", string(content))
				log.Fatal(err)
			}
		} else {
			log.Fatal(err)
		}

		*tmpl = template.New(in)
		files = append(files, in)
		files = append(files, wants...)
		*tmpl, err = (*tmpl).Funcs(build.FuncMap).ParseFiles(files...)
		if err != nil {
			log.Fatal(err)
		}
		err = (*tmpl).Execute(outFile, context)
		if err != nil {
			log.Fatal(err)
		}

		//fmt.Printf("in is %s, out is %s, context is %s, files to include are %v\n", in, out, contextFile, files)
		outFile.Close()
	}
}

func buildDirs(tmpl **template.Template, build data) {
	wants := build.BuildDirWants
	var context map[string]string
	var files []string

	for dir, special := range build.BuildDirTemplates {
		dirFile, err := os.Open(dir)
		if err != nil {
			log.Fatal(err)
		}
		defer dirFile.Close()

		contents, err := dirFile.Readdirnames(-1)
		if err != nil {
			log.Fatal(err)
		}

		for _, file := range contents {
			shortFile := file
			file = filepath.Join(dir, file)
			split := strings.Split(file, `.`)
			if len(split) <= 1 {
				log.Fatal(fmt.Errorf("Input template \"%s\" poorly named, contains no '.'", file))
			} else if split[len(split)-1] != `tmpl` {
				continue
			}

			out := strings.Join(split[0:len(split)-1], `.`)

			outFile, err := os.OpenFile(filepath.Join(build.OutputDir, out), os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0644)
			if err != nil {
				log.Fatal(err)
			}
			defer outFile.Close()

			contextFile := out + ".json"
			if _, err := os.Stat(contextFile); os.IsNotExist(err) {
				context = nil
			} else if err == nil {
				content, err := os.ReadFile(contextFile)
				if err != nil {
					log.Fatal(err)
				}
				err = json.Unmarshal(content, &context)
				if err != nil {
					log.Fatal(err)
				}
			} else {
				log.Fatal(err)
			}

			*tmpl = template.New(shortFile)
			files = special[file]
			files = append(files, file)
			files = append(files, wants...)
			*tmpl, err = (*tmpl).Funcs(build.FuncMap).ParseFiles(files...)
			if err != nil {
				log.Fatal(err)
			}
			err = (*tmpl).Execute(outFile, context)
			if err != nil {
				log.Fatal(err)
			}

			//fmt.Printf("file is %s, out is %s, context is %s, files to include are %v\n", file, out, contextFile, files)
			outFile.Close()
		}

		dirFile.Close()
	}
}

func main() {
	tmpl := template.New("")
	funcs, err := funcMap(&tmpl)
	if err != nil {
		log.Fatal(err)
	}

	var build data
	config, err := os.ReadFile("./data.json")
	if err != nil {
		log.Fatal(err)
	}
	err = json.Unmarshal(config, &build)
	build.FuncMap = funcs

	if build.SourceDir == "" {
		log.Fatal(fmt.Errorf("Sourcedir not set in json config, cannot read in anything"))
	}
	build.SourceDir, err = filepath.Abs(build.SourceDir)
	if err != nil {
		log.Fatal(err)
	}

	if build.OutputDir == "" {
		log.Fatal(fmt.Errorf("OutputDir not set in json config, cannot output"))
	}
	build.OutputDir, err = filepath.Abs(build.OutputDir)
	if err != nil {
		log.Fatal(err)
	}
	if os.Chdir(build.SourceDir) != nil {
		log.Fatal(err)
	}

	buildFiles(&tmpl, build)
	buildDirs(&tmpl, build)
}
