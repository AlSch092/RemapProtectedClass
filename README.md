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

To protect a class, call `RemapClassToProtectedClass`. When you are finished with the class, simply call `UnmapProtectedClass`.

## Where this shouldn't be used:
- Classes using inheritance or virtual functions/vtables
- Complex behaviors such as polymorphism

## Why this works:
- Class/struct members are treated as offsets to the compiler, and we make a direct copy of the class object into our view, thus we can still access members since member offsets will be identical.

## Requirements:
- Make sure to link `ntdll.lib` under Linker -> Input before compiling.
   
Thanks to changeofpace for the original self-remapping-code example, as this project is an application of it

![Example](https://github.com/user-attachments/assets/d9374bd8-2773-4fed-ab97-297a63c01b43)
