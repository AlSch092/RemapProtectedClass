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

    void PrintMembers() //example function showing class functions can still be called and `this` can be used after mapping
    {
        printf("protTest->GameTickSpeed: %d\n", this->GameTickSpeed);
        printf("protTest->GameEngineGravity: %f\n", this->GameEngineGravity);
        printf("protTest->Invincible: %d\n", this->Invincible);
    }
};
#pragma pack(pop)

/*
    RemapClassToProtectedClass - protects a class/structure from memory writes and having its page protections modified
    return `true` on success
*/
template<typename T>
BOOL RemapClassToProtectedClass(T& classPtr)
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
void UnmapProtectedClass(LPCVOID ProtectedClass)
{
    UnmapViewOfFile((LPCVOID)&ProtectedClass);
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

    RemapClassToProtectedClass(protTest); //remap protTest as a protected view

    protTest->PrintMembers(); //class-member call example after protecting class

    UnmapProtectedClass(protTest); //free memory through unmapping

    system("pause");
    return 0;
}
