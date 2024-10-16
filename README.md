# RemapProtectedClass
Remaps a class/struct with `SEC_NO_CHANGE` and `PAGE_EXECUTE_READ`, protecting it from memory modifications and page protection modifications. Ideal for class variables which should not be tampered or modified after being initialized at runtime. Works with heap memory when creating class or structure pointers. As opposed to working on static memory/image sections, this works for variables which can be set at run-time. The technique works for both 32-bit and 64-bit compilations. This is an experimental technique and should only be seen as a proof of concept currently.

## How it works:
1. A class or structure pointer is first created, and its member variables are set to some values.
2. A section is created with flags `SEC_COMMIT` + `SEC_NO_CHANGE`
3. A memory-mapped view is created of this new section with the ViewUnmap enum
4. Class members are all copied to the view
5. The view is then unmapped, and then mapped once more with the `SEC_NO_CHANGE` flag
6. The original class pointer is deleted/memory freed
7. The class pointer's address is set to the mapped view, allowing you to access class members the same way as any normal class pointer
8. The view is later unmapped when you are finished with the class  

To protect a class, call `MapClassToProtectedClass`. When you are finished with the class, simply call `UnmapProtectedClass`.

## Where this shouldn't be used:
- Classes using inheritance or virtual functions/vtables
- Complex behaviors such as polymorphism

## Why this works:
- Class/struct members are treated as offsets to the compiler, and we make a direct copy of the class object into our view, thus we can still access members since member offsets will be identical.

## Modifying values after mapping:
- It is possible to change 'protected' member values if necessary:
1. By creating a 2nd class object, and copying the first's members to the second
2. Modifying any values you need to
3. Then unmapping the first, and mapping the second one
4. And lastly, set the first class pointer to the second one to give the illusion that values can be modified, which allows the user to continue to use the modified first pointer in their codebase.

## Requirements & Warnings:
- Make sure to link `ntdll.lib` under Linker -> Input before compiling.
- Possibly dangerous in a multi-threaded scenario as we are manipulating pointers and memory on the fly; you may want to create critical sections when accessing members of the 'protected' class.
   
Thanks to changeofpace for the original self-remapping-code example, as this project is an idea based off of it

![Example](https://github.com/user-attachments/assets/a9c66139-3f0a-4509-9337-b22c1cf50b67)

