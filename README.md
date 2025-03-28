# MapProtectedClass
Manipulates memory to create a mapped view of a section with attributes `SEC_NO_CHANGE` and `PAGE_EXECUTE_READ`, protecting a class/struct object from memory modifications and page protection modifications. Ideal for classes containing variables which should not be tampered by outside users, or modified often after being set at runtime. This works with heap memory when creating new class/struct objects; As opposed to working on static memory/image sections, this technique works for memory which can be set at runtime residing in the heap. The technique works for both 32-bit and 64-bit compilations. It has been tested in user-facing software with no issues. It is ideal for things like "settings" set at runtime in anti-cheat or anti-malware systems, as it prevents both memory writes and page protection modifications.

The example code uses a RAII class with the 'placement new' C++ concept to use the mapped view's memory address as the class object's address, while the `MappedMemory` class' RAII aspects handle mapping & unmapping memory. 

## How it works:
1. A class or structure object is first created, and its member variables are set to some initial values.
2. A section is created with flags `SEC_COMMIT` + `SEC_NO_CHANGE`
3. A memory-mapped view is created of the new section with the ViewUnmap enum
4. Memory where the class object/struct resides at is copied to the view
5. The view is then unmapped, and then mapped once more with the `SEC_NO_CHANGE` flag
6. The original class object is deleted/has its memory freed
7. The class pointer's address is set to the mapped view, allowing you to access class members the same way as any normal/"non-protected" class pointer
8. The view is later unmapped when you are finished with the class  

To protect a class object, create an instance of the `MappedMemory` class and then call the `Construct<T>` function with your class type and object. Then call the `Protect` function of the `MappedMemory` object to map a view of a section representing your class object. The memory will automatically be unmapped when it goes out of scope due to the `MappedMemory` destructor.

## Where this shouldn't be used:
- Classes using inheritance or virtual functions/vtables
- Complex behaviors such as runtime polymorphism

## Why this works:
- Class/struct members are treated as offsets to the compiler, and we make a direct copy of the class object into our view, thus we can still access members since member offsets will be identical. we are essentially mapping a section with `SEC_NO_CHANGE`, copying our class object it it, and pointing the class object's pointer to that mapped section (and freeing any original memory of the class object)  

## Modifying values after mapping:
- It is possible to change 'protected' member values if necessary:
1. By creating a 2nd class object, and copying the first's members to the second
2. Modifying any values you need to
3. Then unmapping the first, and mapping the second one
4. And lastly, set the first class pointer to the second one to give the illusion that values can be modified, which allows the user to continue to use the modified first pointer in their codebase.

## Requirements & Warnings:
- Make sure to link `ntdll.lib` under Linker -> Input before compiling.
- Possibly dangerous in a multi-threaded scenario as we are manipulating pointers and memory on the fly; you may want to use critical sections when accessing members of the 'protected' class.
   
Thanks to changeofpace for the original self-remapping-code example, as this project is an idea based off of it.

![Example](https://github.com/user-attachments/assets/ea522fc3-e214-4a86-b5d8-8ed51617c750)

