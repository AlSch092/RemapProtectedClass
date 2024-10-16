// RemapClass.cpp : By AlSch092 @ Github
// Thanks to changeofpace @ Github for original self-remapping-code example
// works for both  x86 and x64

#include "RemapClass.hpp"

#pragma pack(push, 1) // Disable padding for good measure
class ProtectedClass
{
public:

    UINT32 GameTickSpeed;
    FLOAT GameEngineGravity;
    BOOL Invincible;

    //...insert other sensitive variables which should not be changed after theyve been initialized

    void PrintMembers()
    {
        printf("ProtectedClass.GameTickSpeed: %d\n", this->GameTickSpeed);
        printf("ProtectedClass.GameEngineGravity: %f\n", this->GameEngineGravity);
        printf("ProtectedClass.Invincible: %d\n", this->Invincible);
    }
};
#pragma pack(pop)

/*
    MapClassToProtectedClass - protects a class/structure from memory writes and having its page protections modified
    return `true` on success. deletes the class object after mapping the view, the mapped view should be freed by you when you're finished with it
*/
template<typename T>
BOOL MapClassToProtectedClass(T& classPtr)
{
    if (classPtr == nullptr)
        return FALSE;

    const SIZE_T sectionSize = sizeof(*classPtr);

    PVOID pViewBase = NULL;
    HANDLE hSection = NULL;
    LARGE_INTEGER cbSectionSize = {};
    LARGE_INTEGER cbSectionOffset = {};
    SIZE_T cbViewSize = 0;

    cbSectionSize.QuadPart = sectionSize;

    NTSTATUS ntstatus = NtCreateSection(&hSection, SECTION_ALL_ACCESS, NULL, &cbSectionSize, PAGE_EXECUTE_READWRITE, SEC_COMMIT | SEC_NO_CHANGE, NULL);

    if (!NT_SUCCESS(ntstatus))
    {
        printf("NtCreateSection failed: 0x%X\n", ntstatus);
        return FALSE;
    }

    ntstatus = NtMapViewOfSection(hSection, NtCurrentProcess(), &pViewBase, 0, PAGE_SIZE, &cbSectionOffset, &cbViewSize, ViewUnmap, 0, PAGE_READWRITE); //create a memory-mapped view of the section

    memcpy((void*)pViewBase, (const void*)classPtr, sizeof(*classPtr)); //copy class member data to the view

#ifdef _WIN64
    printf("ViewBase at: %llX\n", (UINT_PTR)pViewBase);
#else
    printf("ViewBase at: %X\n", (UINT_PTR)pViewBase);
#endif

    ntstatus = NtUnmapViewOfSection(NtCurrentProcess(), pViewBase); //unmap original view

    ntstatus = NtMapViewOfSection(hSection, NtCurrentProcess(), &pViewBase, 0, 0, &cbSectionOffset, &cbViewSize, ViewUnmap, SEC_NO_CHANGE, PAGE_EXECUTE_READ); //remap with SEC_NO_CHANGE

    CloseHandle(hSection);

#ifdef _WIN64
    printf("Mapped as protected at: %llX (unmodifiable memory)\n", (UINT_PTR)pViewBase);
#else
    printf("Mapped as protected at: %X (unmodifiable memory)\n", (UINT_PTR)pViewBase);
#endif

    delete classPtr; //delete original class memory as its no longer needed

    classPtr = (T)pViewBase; //Set class pointer to viewBase, our class is now 'protected' and cannot have its memory changed or page protections modified

    return TRUE;
}

/*
    UnmapProtectedClass - UnmapViewOfFile wrapper
*/
bool UnmapProtectedClass(LPCVOID ProtectedClass)
{
    return UnmapViewOfFile((LPCVOID)ProtectedClass);
}

int main()
{
    ProtectedClass* protTest = new ProtectedClass(); //allocates a class object on the heap, outside of module's image

#ifdef _WIN64
    printf("protTest original address: %llX\n", (UINT_PTR)protTest);
#else
    printf("protTest original address: %X\n", (UINT_PTR)protTest);
#endif

    protTest->GameTickSpeed = 100;
    protTest->GameEngineGravity = 500.0f;
    protTest->Invincible = TRUE; //for the sake of easily viewing a '1' in memory instead of a '0'

    MapClassToProtectedClass(protTest); //remap protTest as a protected view

    //...attempting to write to protTest members will now throw a write violation, otherwise you can verify manually in a debugger that members cannot be written
    // if you need to edit the values of the protected class, you can copy its object to a new class pointer, edit those values, then call `MapClassToProtectedClass` on the new pointer and unmap the old one.

    protTest->PrintMembers(); //class-member call example after protecting class

    //example of 'changing values' - create a new class object as a copy of the old one, change values, then call MapClassToProtectedClass again
    printf("========= Example of modifying values of protTest: ===========\n");

    ProtectedClass* protTest_modified = new ProtectedClass(); //we will modify this pointer, map it as protected memory, and then set the first class pointer to this one to give the illusion of being able to still modify contents
    memcpy((void*)protTest_modified, (const void*)protTest, sizeof(*protTest)); //copy class contents to new class object

#ifdef _WIN64
    printf("protTest_modified  address: %llX\n", (UINT_PTR)protTest_modified);
#else
    printf("protTest_modified  address: %X\n", (UINT_PTR)protTest_modified);
#endif

    protTest_modified->GameTickSpeed = 1337.0f; //...modify whichever member values you need
    protTest_modified->GameEngineGravity = 123;

    UnmapProtectedClass(protTest); // then free memory through unmapping - clear the original protected view since we're updating its values

    MapClassToProtectedClass(protTest_modified); //often, this will map at the same address which the first class pointer was mapped at, convenient but not always guaranteed

    protTest = protTest_modified; //set the original pointer to the new pointer, incase we need to continue using the original pointer in our code. remember that this must keep multithreading in mind and work atomically, you may want to use critical sections in code referencing these variables

    protTest_modified = nullptr; //we no longer need the 2nd pointer since protTest will point to its address

    protTest->PrintMembers(); //will now print the updated values - at this point, protTest == protTest_modified

    UnmapProtectedClass(protTest); //free memory/unmap again, as we are finished with the program
    protTest = nullptr; //cleanup

    system("pause");
    return 0;
}
