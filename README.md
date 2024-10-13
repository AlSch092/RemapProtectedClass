# RemapProtectedClass
Remaps a class/struct with SEC_NO_CHANGE, protecting it from memory modifications. Ideal for class variables which should not be tampered or modified after being initialized. Works with heap memory when creating class or structure pointers.

## How it works:
1. A class or structure pointer is first created, and its member variables are set to some values.
2. A section is created with flags SEC_COMMIT + SEC_NO_CHANGE
3. A memory-mapped view is created of this new section with the ViewUnmap enum
4. Class members are all copied to the view
5. The view is then unmapped, and then mapped once more with the SEC_NO_CHANGE flag
6. The original class pointer is deleted/memory freed
7. The class pointer's address is set to the mapped view, allowing you to access class members the same way as any normal class pointer
8. The view is later unmapped when you are finished with the class  

To protect a class, call `RemapClassToProtectedClass`. When you are finished with the class, simply call `UnmapProtectedClass`.
   
Thanks to changeofpace for the original self-remapping-code example, as this project is an application of it

![Example](https://github.com/user-attachments/assets/d9374bd8-2773-4fed-ab97-297a63c01b43)
