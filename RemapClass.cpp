// RemapClass.cpp : By AlSch092 @ Github
// Thanks to changeofpace @ Github for original self-remapping-code example
// works for both  x86 and x64

#include "RemapClass.hpp"

#pragma pack(push, 1) //disable padding for good measure...
class ProtectedClass 
{
public:
    UINT32 GameTickSpeed;
    FLOAT GameEngineGravity;
    BOOL Invincible;

    ProtectedClass(UINT32 tickSpeed, FLOAT gravity, BOOL invincible)
        : GameTickSpeed(tickSpeed), GameEngineGravity(gravity), Invincible(invincible) {}

    void PrintMembers() const 
    {
        printf("ProtectedClass.GameTickSpeed: %d\n", this->GameTickSpeed);
        printf("ProtectedClass.GameEngineGravity: %f\n", this->GameEngineGravity);
        printf("ProtectedClass.Invincible: %d\n", this->Invincible);
    }
};
#pragma pack(pop)


class MappedMemory   //RAII class to handle memory mapping
{
private:
    HANDLE hSection;
    PVOID pViewBase;
    SIZE_T size;

public:
    MappedMemory(SIZE_T sectionSize) : hSection(nullptr), pViewBase(nullptr), size(sectionSize) 
    {
        LARGE_INTEGER sectionSizeLi = {};
        sectionSizeLi.QuadPart = sectionSize;

        NTSTATUS ntstatus = NtCreateSection(&hSection, SECTION_ALL_ACCESS, NULL, &sectionSizeLi, PAGE_EXECUTE_READWRITE, SEC_COMMIT | SEC_NO_CHANGE, NULL);
        if (!NT_SUCCESS(ntstatus)) 
        {
            throw std::runtime_error("NtCreateSection failed");
        }

        SIZE_T viewSize = 0;
        LARGE_INTEGER sectionOffset = {};

        ntstatus = NtMapViewOfSection(hSection, NtCurrentProcess(), &pViewBase, 0, PAGE_SIZE, &sectionOffset, &viewSize, ViewUnmap, 0, PAGE_READWRITE);
        if (!NT_SUCCESS(ntstatus)) 
        {
            CloseHandle(hSection);
            throw std::runtime_error("NtMapViewOfSection failed");
        }

        printf("Viewbase at: %llX\n", (UINT64)pViewBase);
    }

    
    void Protect()  //protect the memory to make it non-writable
    {
        SIZE_T viewSize = 0;
        LARGE_INTEGER sectionOffset = {};
        NTSTATUS ntstatus = NtUnmapViewOfSection(NtCurrentProcess(), pViewBase); // unmap original view

        ntstatus = NtMapViewOfSection(hSection, NtCurrentProcess(), &pViewBase, 0, 0, &sectionOffset, &viewSize, ViewUnmap, SEC_NO_CHANGE, PAGE_EXECUTE_READ); //map with SEC_NO_CHANGE
        if (!NT_SUCCESS(ntstatus)) 
        {
            throw std::runtime_error("Failed to remap view as protected");
        }
    }

    ~MappedMemory() //RAII destructor - unmap view of section
    {
        if (pViewBase) 
        {
            NtUnmapViewOfSection(NtCurrentProcess(), pViewBase);
        }
        if (hSection) 
        {
            CloseHandle(hSection);
        }
    }

    PVOID GetBaseAddress() const 
    {
        return pViewBase;
    }

    template<typename T, typename... Args>      //"placement new" concept using variadic template
    T* Construct(Args&&... args) 
    {
        return new (pViewBase) T(std::forward<Args>(args)...);
    }
};

int main() 
{
    const SIZE_T classSize = sizeof(ProtectedClass);

    MappedMemory memory(classSize);     //create the memory-mapped view and construct the object inside it using placement new
    ProtectedClass* protTest = memory.Construct<ProtectedClass>(100, 500.0f, TRUE);

    protTest->PrintMembers(); //print initialized values for the sake of showing that we can modify them later if needed by re-mapping

    memory.Protect(); //protect the memory from further writes

    printf("\n========= Example of modifying values (using a new instance) =========\n");

    MappedMemory modifiedMemory(classSize); //create new instance and modify the values
    ProtectedClass* protTestModified = modifiedMemory.Construct<ProtectedClass>(*protTest); //copy the original values

    protTestModified->GameTickSpeed = 1337;   //modify values in the new protected instance
    protTestModified->GameEngineGravity = 123.0f;

    memory.~MappedMemory();  //explicitly call destructor to unmap the original view

    protTest = protTestModified; //reassign the original pointer to the modified view
 
    modifiedMemory.Protect();  //protect the new instance
   
    protTest->PrintMembers(); //print the modified values using the original pointer to give the illusion of being able to continue using the original pointer

    system("pause");
    return 0;
}
