# RASS Coding Standard

The purpose of this document is to provide a standard for formatting, naming, and other situations to provide consistency throughout the project's code.  This helps the maintainability and readability of the project.

Some guidelines also provide some good practices to follow, as learned from past professional experiences.  Where possible, we'll make our best attempts to explain why certain practices are in place.

## Golden Rules

1. If a code already follows a specific coding standard that's different from what's described on this document, follow what's already established on that file instead of following this document.
	* Corollary: *strongly* consider making a low-priority Github issue to update that code's coding standard to follow this document's standard.
2. If any new standards are devised, add those new rules into the `.editorconfig` file *and* import said file onto Visual Studio or Jetbrains Rider to confirm it works.
	* Also, if you haven't done so already, check if the [Editor Config plugin](https://editorconfig.org/) is already installed on the IDE-of-choice.  Note that Visual Studio 2026 already has this built-in.
3. While this document is a "standard" and should be followed to the best of one's abilities, exceptions always exists.  So long as the spirit of this document is followed the grand majority of the time, we consider rare exceptions to be acceptable (and even necessary.)
	* All of these are "guidelines," not "rules" set-to-stone.

Without further ado, the project standards are as follows:

# Text Files (generally)

This section covers general text file encoding-related standards.

#### File & Folder Names

Generally, all file and folder names should consist only of:

* alpha-numeric characters, i.e. `a-z`, `A-Z`, and `0-9`
* the following symbols: `.`, `-`, and/or `_`

Spaces often complicates file-parsing and -handling, so it's better to avoid them when naming a file or folder.  Instead, favor Pascal-casing, e.g.

```cpp
// Good
Version-1.0/ASpecialFile.zip
```

```cpp
// Bad
Version 1.0/A special file.zip
```

If a folder or file name contains a date or time, then the following format is recommended:

```cpp
// With date, only (note, MM and DD for single-digits
// should start with 0, e.g. 01 for January)
FileName-YYYY-MM-DD.zip

// With date and time (mm stands for minutes)
FileName-YYYY-MM-DD-HH-mm.zip
```

This helps with sorting files alphabetically.

#### File Extensions

Generally, all file extensions should be in lowercase.

* Header files should have the `*.h` file extension, while C++ files should have the `*.cpp` file extension; don't use `*.cxx` or `*.c++`.
	* Where possible, the header and the corresponding C++ file should be placed in the same folder.  Do not separate them with a "header" folder and "source" folder.
* JSON files should have the `*.json` file extension.
* JPEG files should have the `*.jpg` file extension; don't use `*.jpeg`.

#### Encoding

All code, JSON, and other text files our game will be parsing should be encoded in UTF-8 format, unless more East Asian language characters are needed (in which case, consider UTF-16 or -32.)

#### EOL (aka Newlines)

For Windows, newlines in text files should be in CRLF format.  For other operating systems, they should be in LF format.  Note that this configuration can be defined on Git's `.gitattributes` file, where it'll auto-translate the newline format to the appropriate operating system.

Some notable exceptions:
* Visual Studio projects and solutions files *must* have CRLF EOL format.
* JSON, HTML, CSS, XML, YAML, and SVG(Z) files *must* have LF EOL format.

On top of defining these exceptions into `.gitattributes`, tt's also recommended to update the `.editorconfig` file to include these exceptions.

In addition, text files should always end with an empty line at the end.  This maximizes compiler support (though this limitation largely applies to outdated software.)

```cpp
// Good (notice the extra line at the end of the file)
class Example {
};

```

```cpp
// Bad
class Example {
};
```

#### Indentation

For indentation, tabs should be favored over spaces.  Tabs provide a developer to customize the number of spacing they take to their liking.  This is considered an advantage.

#### White-space

A line should not end with any whitespaces besides EOL.  If there are tabs or spaces at the end of the line, strongly consider trimming them.

```cpp
// Good (no tabs or spaces at the end of the line)
int yay = 0;
```

```cpp
// Bad
int yay = 0;\t\t
```

# C++ Standards

## Class names

Name classes and structs under PascalCasing.

```cpp
// Good
class Example {
};
```

```cpp
// Bad
class anExample1 { // don't start with a lowercase
};

// Bad
struct An_Example_2 { // don't snake-case
};

// Bad
class _Example3 { // don't start with an underscore
};
```

## Variable names

In general, name variables under camelCasing.  Note that we'll be naming some exceptions in the later subsections, though.

```cpp
// Good
int anExample;
```

```cpp
// Bad
int AnExample1; // don't start with an uppercase
float an_Example_2; // don't snake-case
std::string _example3 // if not a member variables, don't start with an underscore
```

### Exception 1: for non-public member variables, also prepend an underscore

Along with using camelCasing, also start `private` or `protected` member variable names with an underscore, *even if they're constants*.

```cpp
// Good
class Example {
	int _memberVar; // need to start with underscore
}
```

```cpp
// Bad
class Example {
	int memberVar; // need to start with underscore
}
```

### Exception 2: for constants, use ALL_CAPS_SNAKE_CASING

For constant variables, use all-caps, snake_casing.

```cpp
// Good
static const int GLOBAL_VAR;
class Example {
	static const int _MEMBER_VAR;
public:
	static const int PUBLIC_VAR; // public member variables may start without an underscore
}
```

```cpp
// Bad
static const int globalVar; // not all-caps
class Example {
	static const int _memberVar; // not all-caps
}
```

## Function & method names

Name functions and methods under PascalCasing, regardless of whether they're `private`, `public`, or `const`.

```cpp
// Good
void GoodExample1();  // don't start with a lowercase
class Neat {
	void GoodExample2() const;  // don't start with a lowercase
};
```

```cpp
// Bad
void badExample1();  // don't start with a lowercase
class Neat {
	void badExample2() const;  // don't start with a lowercase
};
```

## Acronyms

If any of the above needs to include an acronym in their name, have the first letter of acronym follow the respective naming convention, while the rest be lowercase.  This helps with the legibility.

```cpp
// Good (LOB = line-of-bearing)
void GetLob();
class Neat {
	glm::vector3 lob;
};
```

```cpp
// Bad
void GetLOB();  // don't all-caps acronyms
class Neat {
	glm::vector3 LOB; // don't all-caps acronyms
};
```

Exception: for `const` variables, acronyms may remain in all-caps.

## Favor [K&R](https://en.wikipedia.org/wiki/Indentation_style#K&R) Brackets

For reducing the number of lines and giving a more accurate line-count changes, favor using the [K&R](https://en.wikipedia.org/wiki/Indentation_style#K&R) notation (sometimes rather derogatively known as "Egyption Brackets").  Make sure there is a space between the class/conditional and the opening bracket

```cpp
// Good
class Example {
	int memberVar;
};
```

```cpp
// Bad
class Example1 // Don't use Allman notation
{
	int memberVar;
};

// Bad
class Example2{ // Space needed for open-bracket
	int memberVar;
};
```

This applies to conditionals and loops as well.

```cpp
// Good
if (bool1) {
	Do1();
} else if (bool2) {
	Do2();
} else {
	Do3();
}

// Good
while(bool2) {
	Do2();
}
```

```cpp
// Bad
if (bool1) // Don't use Allman notation
{
	Do1();
}
else if (bool2)
{
	Do2();
}
else
{
	Do3();
}

// Bad
if (bool1){
}else if (bool2){ // illegible spacing
}else{ // illegible spacing
}
```

## One-line conditionals and loops

Even if there is only one line in a conditional or loop, use brackets.

```cpp
// Good
if (bool1) {
	Do1();
}

// Good
while (bool2) {
	Do2();
}
```

```cpp
// Bad
if (bool1) // no brackets
	Do1();

// Bad
while (bool2) // no brackets
	Do2();
```

## Switch-case

For indenting switch-case statements, have `case`-lines in the same indentation as `switch`.

```cpp
// Good
switch(test) {
case 0:
	Do1();
	break;
case 2:
case 3:
	Do2();
	break;
default:
	break;
}
```

## Macros and scope

Macros and scope-based keywords should have no indentation.

```cpp
// Good
class Example {
	int member1;
#ifdef DEBUG
	int member2;
#endif

public:
	void Do1();
}
```

## `#include`

Per [ISO Standard](https://isocpp.org/wiki/faq/Coding-standards#std-headers), for importing C and standard library files, favor the `#include <cxxx>` notation.
 instead of `#include <xxx.h>` or `#include "xxx.h"`

```cpp
// Good
#include <xxx>
#include <cxxx>
```

```cpp
// Bad
#include <xxx.h> // no need for *.h
#include "xxx.h" // indicative the header is a local file,
                 // which for standard libraries, is false
```

## "Interface" classes

While C++ doesn't technically support interfaces, RASS project names virtual function-only classes by prepending the letter, `I`.

```cpp
// Good
class IExample {
	virtual void Do1() = 0;
	virtual void Do2() = 0;
}
```

## Multiline conditionals and arrays

When defining long conditionals or array declaration that takes up multiple lines, have the operator or comma, respectively, as the start of the next line (along with obviously the extra indentation.)  This makes copy-paste easier, and less error-prone.

```cpp
// Desirable
if (bool1
	&& bool2) {
}

// Desirable
std::vector list = {
	element1
	, element2 // easier to copy-paste.
	           // Also better for JSON files.
}
```

```cpp
// Unfavorable
if (bool1 && // don't end with the operation
	bool2) {
}

// Unfavorable
std::vector list = {
	element1, // don't end with a comma
	element2
}
```

Granted, some exceptions exist where this notation is less legible (e.g. conditionals with a lot of paranthesis).  Try following where it makes sense, and break where it doesn't.

## Internal commenting

Avoid using the `/* */` notation for commenting.  It's error-prone, as developers tends to accidentally nest `/* */` inside another `/* */` (e.g. through git-merge,) which defeats the notation's purpose.  If a comment spans multiple lines, use the IDE's shortcut (e.g. Visual Studio's Ctrl+K, Ctrl+C shortcut) to bulk-convert that set of text to single-line-comments.

```cpp
// Good
if (bool1 && bool2) {
	// I thought long and hard
	// on what to add for this conditional
}
```

```cpp
// Bad
if (bool1 && bool2) {
	/* I thought long and hard
	on what to add for this conditional */
}
```

There are some acceptable exceptions, however:

### Exception 1: documentation

RASS project uses the Doxygen standard for documenting classes and functions.  In these cases, the `/** */` notation is used.

```cpp
// Good
/**
 * Performs the default action.
 * @param id this function caller's identification number.
 */
void Do1(int id);
```

### Exception 2: mid-line comments

Sometimes, it makes sense to add a comment mid-line.  If that's the case, `/* */` is the only option available, and thus a valid situation to use.

```cpp
// Good
if (bool1 /* Should be true if this is the first time running */ || bool2) {
}
```

## For classes: favor composition, *not* extension

Extending classes often creates complexity and memory load on the developer, making the code less readable.  Where possible, prefer holding the instance of an object as a method of adding more capabilities to it, than extending the class itself.

```cpp
// Desirable
class Derived {
	private BaseClass& base;
public:
	public Derived(BaseClass& toExtend) : base(toExtend) {
	}
	// Add extension methods to BaseClass
}
```

```cpp
// Unfavorable
class Derived : public BaseClass {
	// Add extension methods to BaseClass
}
```

## Namespaces

To avoid naming conflicts, *all* classes and structs should be defined under a namespace.  Classes in a namespace can be in the same indentation as the namespace block.

```cpp
// Good
namespace RassEngine {
class TimeSystem {
};
}
```

## Header files

### No `using namespace`

For header code, do *not* use the `using namespace` shortcut.  If the lines of code ends up being really long, strongly consider refactoring to avoid this situation.

Note that one *may* use `using namespace` shortcut in the C++ source code.

### Prefer forward declaration over `#include`

Forward declarations leads to faster compilation, and helps resolve circular referencing (rather common issue if using `#include`.) Unless the include is for standard libraries, forward declataion is less error-prone, and preferred.

### Class structure

Below lists the order of which the RASS teams organizes each class' data:

1. Define any forward declarations within the class, first.
2. Next, list all public `static const` variables.
4. List all functions and methods:
	1. Prioritize listing public methods first, in the following sub-order:
		1. `static` functions.
		2. constructors and destructors.
		3. abstract methods, i.e. any `virtual` methods ending with `= 0;`.
		4. `virtual` methods.
		5. `inline const` methods, i.e. methods that doesn't change the class' member variables.
		6. `inline` methods.
			* Exception: if there's a matching pair of getters and setters, each touching the same variable; group them together, regardless of the above sub-order.
		7. `const` methods, i.e. methods that doesn't change the class' member variables.
		8. Finally list the rest.
			* Exception: if there's a matching pair of getters and setters, each touching the same variable; group them together, regardless of the above sub-order.
	5. List all protected functions.
		* Follows the same sub-order as public functions.
	6. List all private functions.
		* Follows the same sub-order as public functions.
7. List the rest of the member variables.
	1. List public `const` member variables, first.
		* *All* public member variables should be `const`.
	2. Next, list protected member variables, under the following sub-order
		1. `static const` variables.
		2. `static` variables.
		3. `const` variables.
		4. Finally, list the rest
	3. List private member variables, last.
		* Follows the same sub-order as protected variables.

Example:
```cpp
// Desirable
class Derived {
	// forward declaration
	class Base;

public:
	// public statics
	static const int STATIC_CONST_INT = 0;
	static int CalculateSomething() { return 2; }

	// constructors/destructors
	Derived() {
	}
	virtual ~Derived() {
	}
	Derived(const Derived&) = delete;

	// virtual methods
	virtual void doAbstract() = 0;
	virtual void doOverride();

	// inline methods
	inline int get1() const { return 1; }
	inline void setSome(int some) { this.anInt = some; }

	// methods
	int getOther() const;
	void setOther(int other);

protected:
	// virtual methods
	virtual void doAbstract2() = 0;
	virtual void doOverride2();

	// inline methods
	inline int get2() const { return 2; }
	inline void setSome2(int some2) { this.aMutable = some2; }

	// methods
	int getOther2() const;
	void setOther2(int other2);

private:
	// virtual methods
	virtual void doAbstract3() = 0;
	virtual void doOverride3();

	// inline methods
	inline int get3() const { return 3; }
	inline void setSome3(int some3) { this.anInt2 = some3; }

	// methods
	int getOther3() const;
	void setOther3(int other3);

public:
	// public const
	const int CONST_INT = 0;

protected:
	static const int _STATIC_CONST_INT_2 = 2;
	static int _staticInt = 2;
	const int _CONST_INT_2 = 2;
	int anInt = 2;
	mutable int aMutable = 2;

private:
	static const int _STATIC_CONST_INT_3 = 3;
	static int _staticInt2 = 3;
	const int _CONST_INT_3 = 3;
	int anInt2 = 3;
	mutable int aMutable2 = 3;
}
```

# Common Practice

## Line limits

These aren't hard rules per se, but still *very* strong preferences:

* Keep each file under 300 *effective* lines long.
	* Obviously, long documentation comments are reasonable exceptions that can be discounted from this limit.
	* If it ends up longer, use the `#region` and `#endregion` macro liberally
* The *grand* majority of the content of functions/methods should be at least 3-lines
	* If less than that, ask if it really should be a function or not.
* Keep each line under 120 characters long.
	* Given all IDEs have line-wraps, though, this is the *least*-stringent guideline we have in this section.

## Use the linter!

Linters help find bad coding practices.  Take advantage of them!

## Fix warnings: they are not suggestions

Warnings exists to indicate something is not written as an API expects it.  If the break in standard is deliberate or necessary, then hide that warning, *scoped to those exceptional lines*.

Otherwise, do *not* hide or ignore a warning you do not understand.  The internet exists: discover the real reason they appear and address accordingly.

Note: it's possible to configure some IDEs to treat warnings as errors.  It's strongly recommended to enable this feature.

## NEVER make non-constant member variables in a class `public`

Define getters & setters instead.  This drastically helps in a multi-threaded application, especially when multiple threads are accessing the same member variable.

Note that member variables to a class *may* be `protected`.  Though even then, it's more favorable to make the getters/setters protected, instead of the variable itself.

The only exceptions to this are `structs`, where all member variables are supposed to be public by default, anyway.

## ALWAYS make destructors virtual

Make sure to mark destructors as virtual so all base class destructors are called.

## Use `const` liberally (even as default)

Where it makes sense, define methods and their parameters as `const` as much as possible.  You can always remove them, if necessary.

Note: sometimes, one might add a performance enchancement through caching, that breaks the `const` method pattern.  Do consider using `mutable` for cache variables, if that's the case.  That said, `mutable` should be used rarely.

## The Flyweight Pattern

A common software design pattern the RASS engine utilizes is the flyweight pattern: where an existing schema of objects are cloned by a factory.  This eases the developer from constantly needing to re-parse a JSON prefab file to recreate the same entity over and over again.

As a consequence, a number of practices unique to this project has been implemented:

### ALWAYS define a clone constructor, or mark them as `= delete`

For the flyweight factory to work, nearly all code (especially `Components`) will need to have their clone constructor defined.  If it's known that a clone constructor will not be needed, please make that explicit by having the `= delete;` added in the header to make that clear.  This would help the compiler find errors if a programmer attempts to factory-clone the object unintentionally.

### Constructors are for defining default member variables, `::Initialize()` is for actual setup

For flyweight objects to work, their constructor *needs* to perform less setup than would be normally expected: the original "schema" entities and components shouldn't bind to any events, for example, as they're not "active" in a game.  Instead, an `Entity` or `Component` object have an `::Initialize()` method to perform the actual initialization, such as binding to common events, assigning values to member variables that couldn't be copied over from the copy constructor, etc.  Perform setup in `::Initialize()` for these set of classes.

## Avoid `else` and `default`; define a default state, instead

A way to alleviate a long if-else or switch-case blocks is to first define the default state, then define the rest of the conditions.  This allows one to avoid using the final else-block:

```cpp
// Desirable
int state = 0; // state is 0 by default
if (bool1) {
	state = 1;
} // No else needed: state is already 0
```

```cpp
// Unfavorable
int state;
if (bool1) {
	state = 1;
} else {
	state = 0;
}
```

Of course, there are plenty of exceptions where else-block is necessary.  Use else as necessary; but where possible, avoid them.

Note: most linters by default require switch-case to have a `default` condition, and most compiler will throw this as a warning.  We believe this practice on avoiding `default`s takes precedence: reconfigure the linter and warning system to ignore this specific condition.

## Avoid nesting if-conditionals and loops: use `return` and `continue` liberally

Where possible, avoid nesting conditionals.  Simply halt the function, or if in loops, skip to the next element, to avoid this nesting.

Nesting requires the developer to memorize all prior conditions.  Reducing this complexity helps make the code easier to read.

```cpp
// Desirable
int Do1() {
	int state = 0;
	if (!bool1) {
		return state;
	}

	state = 1;
	if (!bool2) {
		return state;
	}

	state = 2;
	return state;
}
```

```cpp
// Unfavorable
int Do1() {
	int state = 0;
	if (bool1) {
		state = 1;
		if (bool2) {
			state = 2;
		}
	}
	return state;
}
```

## If a class/variable/function has the word "And", "Or", and/or "Then" in its name, *strongly* consider splitting it into two

Class, variables, and functions should serve a single purpose.  If "and", "or", and/or "then" is in their name, it's likely serving more than one purpose.  Strongly consider refactoring it into smaller parts.

## If a function is *only* called once, *strongly* consider merging it back to the caller

A function called only once indicates it's not really serving a good purpose being a function: a set of lines to be repeated in multiple areas of code.  Only consider leaving it separate if either the function is computing a very common math formula, e.g. square-distance (common exception,) or the function is named well enough that it's more comprehensible than expanding it back to its contents (rare.)

## Avoid calling `new`: use `std::make_unique<>` or `std::make_shared<>`, instead

Pointers are *extremely* error-prone, difficult to track, and easily the main cause of memory leaks and other terrible mistakes.  Use the standard library and utilize the amazing `std::unique_ptr<>` and `std::shared_ptr<>` APIs.  The `new` keyword (let alone `delete`) should be used very rarely, and under exceptional circumstances.

## If something needs to be eventually closed/cleaned-up, take advantage of `std::unique_ptr<>`

Say you have a file-stream opened.  Eventually, it needs to be closed when it's done.  One *could* attempt to close the stream manually, but errors could occur while reading; or maybe the developer returns early without closing the stream.  Can you really cover all potential edge-cases, or at least remember to add try-catch-finally all the time?  Kind of hard, right?

It's `std::unique_ptr<>` to the rescue!  Strongly consider creating a `Handler`-like class that wraps the file-stream instance.  In said `Handler`, have the destructor close the file-stream.  Finally, call `std::make_unique<Handler>` to construct the stream-handler and viola!  You now have a file-stream that auto-closes once it goes out-of-scope, *including* when an error occurs mid-processing.