package main

import (
	"html/template"
	"log"
	"os"
	"strings"
)

func main() {
	var myMap template.FuncMap = map[string]interface{}{}
	myMap["stripLeading"] = func(text string) string {
		split := strings.Split(strings.ReplaceAll(text, "\r\n", "\n"), "\n")

		f := func(c rune) bool { return c != ' ' }
		var out []string
		for _, e := range split {
			offset := strings.IndexFunc(e, f)
			out = append(out, e[offset:])
		}
		return strings.Join(out, "\n") + "\n"
	}

	text := `{{ stripLeading "Happy\n      birthday\n    to\n  you" }}`
	tmpl, err := template.New("birthday").Funcs(myMap).Parse(text)
	if err != nil {
		log.Fatal(err)
	}
	tmpl.ExecuteTemplate(os.Stdout, "birthday", nil)
}
