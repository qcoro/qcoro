def define_env(env):

    @env.macro
    def doctable(module, include, inherits=None, inheritedBy=[]):
        def row(th, td):
            return f"<tr><th>{ th }</th><td>{ td }</td></tr>"

        def inheritsLink(inherits):
            return f"""<a href="../../{inherits[0]}">{inherits[1]}</a>"""

        out = """<div class="doctable"><table>"""
        out += row("Module", module)
        out += row("Include", f"""
```cpp
#include <{include}>
```
""")
        out += row("CMake", f"""
```cpp
target_link_libraries(myapp QCoro::{module})
```
""")
        out += row("QMake", f"""
```cpp
QT += QCoro{module}
```
""")
        if inherits:
            out += row("Inherits", inheritsLink(inherits))

        if inheritedBy:
            out += row("Inherited&nbsp;By", ', '.join(sorted(map(inheritsLink, inheritedBy))))

        out += "</table></div>"
        return out
