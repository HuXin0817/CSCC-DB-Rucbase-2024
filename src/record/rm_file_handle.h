#pragma once

#include <cassert>
#include <memory>

#include "bitmap.h"
#include "common/context.h"
#include "rm_defs.h"

class RmManager;

/* 对表数据文件中的页面进行封装 */
struct RmPageHandle
{
    const RmFileHdr *file_hdr; // 当前页面所在文件的文件头指针
    Page *page;                // 页面的实际数据，包括页面存储的数据、元信息等
    RmPageHdr *page_hdr;       // page->data的第一部分，存储页面元信息，指针指向首地址，长度为sizeof(RmPageHdr)
    char *bitmap;              // page->data的第二部分，存储页面的bitmap，指针指向首地址，长度为file_hdr->bitmap_size
    char *slots;               // page->data的第三部分，存储表的记录，指针指向首地址，每个slot的长度为file_hdr->record_size

    RmPageHandle(const RmFileHdr *fhdr_, Page *page_) : file_hdr(fhdr_), page(page_)
    {
        page_hdr = reinterpret_cast<RmPageHdr *>(page->get_data());
        bitmap = page->get_data() + sizeof(RmPageHdr);
        slots = bitmap + file_hdr->bitmap_size;
    }

    RmPageHandle() = default;

    // 返回指定slot_no的slot存储收地址
    char *get_slot(int slot_no) const
    {
        return slots + slot_no * file_hdr->record_size; // slots的首地址 + slot个数 * 每个slot的大小(每个record的大小)
    }
};

/* 每个RmFileHandle对应一个表的数据文件，里面有多个page，每个page的数据封装在RmPageHandle中 */
class RmFileHandle
{
    friend class RmScan;

    friend class RmManager;

public:
    DiskManager *disk_manager_;
    BufferPoolManager *buffer_pool_manager_;
    int fd_;             // 打开文件后产生的文件句柄
    RmFileHdr file_hdr_; // 文件头，维护当前表文件的元数据
    std::string tab_name_;

    RmFileHandle(DiskManager *disk_manager, BufferPoolManager *buffer_pool_manager, int fd) : disk_manager_(disk_manager), buffer_pool_manager_(buffer_pool_manager), fd_(fd)
    {
        // 注意：这里从磁盘中读出文件描述符为fd的文件的file_hdr，读到内存中
        // 这里实际就是初始化file_hdr，只不过是从磁盘中读出进行初始化
        // init file_hdr_
        disk_manager_->read_page(fd, RM_FILE_HDR_PAGE, (char *)&file_hdr_, sizeof(file_hdr_));
        // disk_manager管理的fd对应的文件中，设置从file_hdr_.num_pages开始分配page_no
        disk_manager_->set_fd2pageno(fd, file_hdr_.num_pages);

        tab_name_ = disk_manager->get_file_name(fd);
    }

    RmFileHdr get_file_hdr() const { return file_hdr_; }

    int GetFd() const { return fd_; }

    /* 判断指定位置上是否已经存在一条记录，通过Bitmap来判断 */
    bool is_record(const Rid &rid) const
    {
        RmPageHandle page_handle = fetch_page_handle(rid.page_no);
        return Bitmap::is_set(page_handle.bitmap, rid.slot_no); // page的slot_no位置上是否有record
    }

    std::unique_ptr<RmRecord> get_record(const Rid &rid, Context *context = nullptr) const;

    Rid insert_record(char *buf, Context *context = nullptr);

    void reset_data_on_rid(const Rid &rid, char *buf);

    void delete_record(const Rid &rid, Context *context = nullptr);

    void update_record(const Rid &rid, char *buf, Context *context = nullptr);

    RmPageHandle create_new_page_handle();

    RmPageHandle fetch_page_handle(int page_no) const;

    size_t record_size() const;

private:
    RmPageHandle create_page_handle();

    void release_page_handle(RmPageHandle &page_handle);
};
