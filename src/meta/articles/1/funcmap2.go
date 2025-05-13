package main

import (
	"bytes"
	"html/template"
	"log"
	"os"
	"strings"
)

func myMap(tmpl **template.Template) template.FuncMap {
	var funcs template.FuncMap = map[string]interface{}{}
	funcs["include"] = func(name string, data interface{},
		times int) (string, error) {
		indent := "\n" + strings.Repeat(" ", times)
		buf := bytes.NewBuffer(nil)
		err := (*tmpl).ExecuteTemplate(buf, name, data)
		if err != nil {
			log.Fatal(err)
		}
		str := buf.String()
		str = strings.Trim(str, "\n")
		split := strings.Split(strings.ReplaceAll(str, "\r\n", "\n"), "\n")
		str = strings.Join(split, indent) + "\n"
		return str, nil
	}
	return funcs
}

func main() {
	included := `{{ define "happy" }}
greetings:
- Happy
- birthday
- to
- you
{{ end }}`
	text := `{{ include "happy" nil 2 }}`
	tmpl := template.New("birthday")
	tmpl = tmpl.Funcs(myMap(&tmpl))
	tmpl, err := tmpl.Parse(included)
	tmpl, err = tmpl.Parse(text)
	if err != nil {
		log.Fatal(err)
	}
	tmpl.Execute(os.Stdout, nil)
}
