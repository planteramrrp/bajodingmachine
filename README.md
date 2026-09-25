# BajodingMachine v0
## Build
Compile with C++ compiler of choice with C++26 standard enabled
## Usage
`./bajodingmachine filename` to compile bajodes
## Bajoding
BajodingMachine code is normal js with HTML elements embedded inside them
```let $count = $state(0);

<button onclick="$count++">text</button>
<p>{$count}</p>

<h1 style="font-size: {$count}px">size</h1>
```
<sup>Given code displays count and appropriately resizes the `<h1>`</sup>
- `{}` syntax can be used element attribute values or content for evaluating code inside them
- prefix reactive variables with `$`
- declaring elements inside js creates that element in DOM body
## Current Limitations
-	Currently no reactive way to bind values of input element to variables \
	This will be addressed in v1
-	`for()`, `if()`, `while()` and others lack reactive counterparts to update when conditions change \
	these structures will be left untouched to prevent js in them from being rerun when conditions change \
	counterpart structures will be introduced in v1 for reactive updating
