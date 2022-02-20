#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include  <Library/MemoryAllocationLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>

struct MemoryMap
{
    UINTN buffer_size;
    VOID* buffer;
    UINTN map_size;
    UINTN map_key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
};

// UEFIの機能を使ってメモリマップを取得する
// UEFIには
//   ブートサービス: OSを起動するために必要な機能を提供する
//   ランタイムサービス: OSの起動前後どちらにでも使える機能を提供する
// の2つがある.
EFI_STATUS GetMemoryMap(struct MemoryMap* map)
{
    if (map->buffer == NULL) {
        return EFI_BUFFER_TOO_SMALL;
    }

    map->map_size = map->buffer_size;

    // ブートサービスからメモリマップを取得する
    // 呼び出し時点のメモリマップを取得し、引数で指定されたメモリ領域に書き込む.
    return gBS->GetMemoryMap(
        &map->map_size,
        (EFI_MEMORY_DESCRIPTOR*)map->buffer,
        &map->map_key,
        &map->descriptor_size,
        &map->descriptor_version
    );
}

const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
  switch (type) {
    case EfiReservedMemoryType: return L"EfiReservedMemoryType";
    case EfiLoaderCode: return L"EfiLoaderCode";
    case EfiLoaderData: return L"EfiLoaderData";
    case EfiBootServicesCode: return L"EfiBootServicesCode";
    case EfiBootServicesData: return L"EfiBootServicesData";
    case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
    case EfiConventionalMemory: return L"EfiConventionalMemory";
    case EfiUnusableMemory: return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode: return L"EfiPalCode";
    case EfiPersistentMemory: return L"EfiPersistentMemory";
    case EfiMaxMemoryType: return L"EfiMaxMemoryType";
    default: return L"InvalidMemoryType";
  }
}

EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file)
{
    EFI_STATUS status;
    CHAR8 buf[256];
    UINTN len;

    CHAR8* header = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
    len = AsciiStrLen(header);
    status = file->Write(file, &len, header);
    if (EFI_ERROR(status)) {
        return status;
    }

    Print(L"map->buffer = %08lx, map->map_size = %08lx\n", map->buffer, map->map_size);

    EFI_PHYSICAL_ADDRESS iter;
    int i;
    for (
        iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
        iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
        iter += map->descriptor_size, i++
    ) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;

        len = AsciiSPrint(
            buf,
            sizeof(buf),
            "%u, %x, %-ls, %08lx, %lx, %lx\n",
            i,
            desc->Type,
            GetMemoryTypeUnicode(desc->Type),
            desc->PhysicalStart,
            desc->NumberOfPages,
            desc->Attribute & 0xffffflu
        );
        status = file->Write(file, &len, buf);
        if(EFI_ERROR(status)) {
            return status;
        }
    }

    return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root)
{
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

    status = gBS->OpenProtocol(
        image_handle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**)&loaded_image,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );
    if(EFI_ERROR(status)) {
        return status;
    }

    status = gBS->OpenProtocol(
        loaded_image->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID**)&fs,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );
    if(EFI_ERROR(status)) {
        return status;
    }

    return fs->OpenVolume(fs, root);
}

EFI_STATUS OpenGOP( EFI_HANDLE image_handle, EFI_GRAPHICS_OUTPUT_PROTOCOL** gop)
{
    EFI_STATUS status;
    UINTN num_gop_handles = 0;
    EFI_HANDLE* gop_handles = NULL;
    status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        &num_gop_handles,
        &gop_handles
    );
    if(EFI_ERROR(status)) {
        return status;
    }

    status = gBS->OpenProtocol(
        gop_handles[0],
        &gEfiGraphicsOutputProtocolGuid,
        (VOID**)gop,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );
    if(EFI_ERROR(status)) {
        return status;
    }

    FreePool(gop_handles);

    return EFI_SUCCESS;
}

const CHAR16* GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt) {
  switch (fmt) {
    case PixelRedGreenBlueReserved8BitPerColor:
      return L"PixelRedGreenBlueReserved8BitPerColor";
    case PixelBlueGreenRedReserved8BitPerColor:
      return L"PixelBlueGreenRedReserved8BitPerColor";
    case PixelBitMask:
      return L"PixelBitMask";
    case PixelBltOnly:
      return L"PixelBltOnly";
    case PixelFormatMax:
      return L"PixelFormatMax";
    default:
      return L"InvalidPixelFormat";
  }
}

void Halt(void) {
    while(1) __asm__("hlt");
}


EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE* system_table
) {
    EFI_STATUS status;

    Print(L"Hello, My MikanOS!\n");

    CHAR8 memmap_buf[1024 * 4 * 4];
    struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
    status = GetMemoryMap(&memmap);
    if (EFI_ERROR(status)) {
        Print(L"failed to get memory map: %r\n", status);
        Halt();
    }

    EFI_FILE_PROTOCOL* root_dir;
    status = OpenRootDir(image_handle, &root_dir);
    if (EFI_ERROR(status)) {
        Print(L"failed to open root directory: %r\n", status);
        Halt();
    }

    EFI_FILE_PROTOCOL* memmap_file;
    status = root_dir->Open(
        root_dir,
        &memmap_file,
        L"\\memmap",
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
        0
    );
    if(EFI_ERROR(status)) {
        Print(L"failed to open file '\\memmap': %r\n", status);
        Print(L"Ignored.\n");
    } else {
        status = SaveMemoryMap(&memmap, memmap_file);
        if (EFI_ERROR(status)) {
            Print(L"failed to save memory map: %r\n", status);
            Halt();
        }
        status = memmap_file->Close(memmap_file);
        if (EFI_ERROR(status)) {
            Print(L"failed to close memory map: %r\n", status);
            Halt();
        }
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    status = OpenGOP(image_handle, &gop);
    if (EFI_ERROR(status)) {
        Print(L"failed to open GOP: %r\n", status);
        Halt();
    }
    Print(
        L"Resoulution: %ux%u, Pixel Format: %s, %u pixels/line\n",
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),
        gop->Mode->Info->PixelsPerScanLine
    );

    Print(
        L"Frame Buffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
        gop->Mode->FrameBufferBase,
        gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,
        gop->Mode->FrameBufferSize
    );

    // カーネルファイルを読み込む
    EFI_FILE_PROTOCOL* kernel_file;
    status = root_dir->Open(
        root_dir,
        &kernel_file,
        L"\\kernel.elf",
        EFI_FILE_MODE_READ,
        0
    );
    if(EFI_ERROR(status)) {
        Print(L"failed to open file '\\kernel.elf': %r\n", status);
        Halt();
    }

    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
    UINT8 file_info_buffer[file_info_size];
    status = kernel_file->GetInfo(
        kernel_file,
        &gEfiFileInfoGuid,
        &file_info_size,
        file_info_buffer
        // file_info_bufferはEFI_FILE_INFO型を十分格納できる大きさのメモリ領域を指定する必要がある.
        // 今回はEFI_FILE_INFO + \kernel.elf の12byte分
    );
    if(EFI_ERROR(status)) {
        Print(L"failed to get file information: %r\n", status);
        Halt();
    }

    // UINT8* -> EFI_FILE_INFO* のキャスト
    EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
    UINTN kernel_file_size = file_info->FileSize;

    EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
    status = gBS->AllocatePages(
        AllocateAddress,
        EfiLoaderData,
        (kernel_file_size + 0xfff) / 0x1000,
        &kernel_base_addr
    );
    if(EFI_ERROR(status)) {
        Print(L"failed to allocate pages: %r\n", status);
        Halt();
    }

    status = kernel_file->Read(kernel_file, &kernel_file_size, (VOID*)kernel_base_addr);
    Print(L"Kernel: 0x%0lx (%lu bytes)\n", kernel_base_addr, kernel_file_size);
    if(EFI_ERROR(status)) {
        Print(L"error: %r\n", status);
        Halt();
    }

    // OSにとってbootserviceは邪魔なので止める.
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if(EFI_ERROR(status)) {
        // ブートサービスを色々利用したので一回目は失敗するだろうけど、メモリマップを取得し直して行う2回目はおそらく成功するだろう.
        status = GetMemoryMap(&memmap);
        if(EFI_ERROR(status)) {
            Print(L"failed to get memory map: %r\n", status);
            Halt();
        }
        status = gBS->ExitBootServices(image_handle, memmap.map_key);
        if(EFI_ERROR(status)) {
            Print(L"Could not exit boot service: %r\n", status);
            Halt();
        }
    }

    UINT64 entry_addr = *(UINT64*)(kernel_base_addr + 24);

    typedef void EntryPointType(UINT64, UINT64);
    EntryPointType* entry_point = (EntryPointType*)entry_addr;
    entry_point(gop->Mode->FrameBufferBase, gop->Mode->FrameBufferSize);


    Print(L"All done\n");

    while (1);
    return EFI_SUCCESS;
}