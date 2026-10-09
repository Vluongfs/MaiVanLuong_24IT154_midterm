# ĐỒ ÁN GIỮA KỲ – TRIỂN KHAI LỆNH `ls(1)` TRÊN NETBSD 10.1

**Học phần:** Lập trình hệ thống (System Programming)  
**Đề tài:** Implement `ls(1)`  
**Sinh viên thực hiện:** Mai Văn Lương  
**Mã số sinh viên:** 24IT154  
**Hệ điều hành mục tiêu:** NetBSD 10.1  
**Tên repository:** `MaiVanLuong_24IT154_midterm`

## 1. Giới thiệu

Dự án xây dựng chương trình dòng lệnh `myls` bằng ngôn ngữ C, mô phỏng các chức năng của tiện ích `ls(1)` trên NetBSD 10.1.

Chương trình liệt kê tập tin/thư mục, xử lý các đối số đường dẫn, cung cấp thông tin chi tiết về tập tin, hỗ trợ sắp xếp theo các tiêu chí, duyệt thư mục đệ quy và xử lý một số loại tập tin đặc biệt. Mục tiêu của dự án là thực hành lập trình hệ thống theo kiến trúc module và sử dụng API hệ thống tập tin trên môi trường UNIX/BSD.

## 2. Môi trường và công cụ

| Thành phần | Thông tin |
| --- | --- |
| Hệ điều hành | NetBSD 10.1 |
| Ngôn ngữ | C (chuẩn C11) |
| Trình biên dịch | `cc` trên NetBSD |
| Công cụ xây dựng | `make` (BSD Make) |
| Quản lý phiên bản | Git |
| Chương trình tham chiếu | `ls(1)` của NetBSD 10.1 |

## 3. Cấu trúc dự án

```text
midterm_ls/
├── .gitignore
├── Makefile
├── README.md
├── include/
│   ├── display.h
│   ├── entries.h
│   ├── filesystem.h
│   ├── options.h
│   └── sorting.h
└── src/
    ├── main.c
    ├── options.c
    ├── filesystem.c
    ├── path.c
    ├── entries.c
    ├── sorting.c
    └── display.c
```

**Vai trò của từng module:**

| Module | Nhiệm vụ |
| --- | --- |
| `main.c` | Điều phối luồng xử lý, tập hợp và phân loại nhiều đối số |
| `options.c` | Phân tích tùy chọn dòng lệnh và cấu hình chế độ hoạt động |
| `filesystem.c` | Đọc thư mục, thu thập các mục và xử lý duyệt đệ quy |
| `path.c` | Phân loại và xử lý từng đối số đường dẫn |
| `entries.c` | Quản lý danh sách phần tử bằng bộ nhớ động |
| `sorting.c` | Sắp xếp các phần tử theo tên, thời gian hoặc kích thước |
| `display.c` | Định dạng tên, quyền, chủ sở hữu, kích thước, thời gian, inode, số khối và loại tập tin |

Các header tương ứng trong thư mục `include/` khai báo kiểu dữ liệu, cấu trúc và giao diện hàm giữa các module.

## 4. Các tùy chọn được hỗ trợ

Cú pháp theo phạm vi triển khai:

```sh
./myls [-AacdFfhiklnqRrSstuw] [file ...]
```

Dự án triển khai **19 tùy chọn** sau:

| Tùy chọn | Chức năng |
| --- | --- |
| `-A` | Hiển thị mục ẩn, ngoại trừ `.` và `..` |
| `-a` | Hiển thị tất cả các mục, kể cả `.` và `..` |
| `-c` | Dùng thời gian thay đổi trạng thái (*ctime*) khi có chế độ thời gian thích hợp |
| `-d` | Hiển thị bản thân đối số thư mục thay vì nội dung bên trong |
| `-F` | Thêm ký hiệu phân loại ở cuối tên tập tin theo loại |
| `-f` | Không sắp xếp các mục theo thứ tự mặc định |
| `-h` | Hiển thị kích thước theo dạng dễ đọc khi kết hợp với chế độ phù hợp |
| `-i` | Hiển thị số inode |
| `-k` | Sử dụng đơn vị khối kilobyte theo quy tắc của tùy chọn |
| `-l` | Hiển thị thông tin tập tin dạng chi tiết |
| `-n` | Hiển thị UID và GID dạng số thay vì tên |
| `-q` | Thay các ký tự không in được trong tên bằng `?` |
| `-R` | Duyệt và liệt kê thư mục theo đệ quy |
| `-r` | Đảo thứ tự sắp xếp |
| `-S` | Sắp xếp theo kích thước tập tin |
| `-s` | Hiển thị số khối |
| `-t` | Sắp xếp theo thời gian |
| `-u` | Dùng thời gian truy cập (*atime*) khi có chế độ thời gian thích hợp |
| `-w` | Giữ nguyên các ký tự trong tên khi hiển thị |

Các tùy chọn có thể kết hợp với nhau và với một hoặc nhiều đối số đường dẫn. Quy tắc tương tác giữa các tùy chọn được kiểm thử bằng cách đối chiếu với `ls(1)` gốc.

## 5. Biên dịch và chạy chương trình

### 5.1. Biên dịch thông thường

Tại thư mục gốc của dự án:

```sh
make
```

File thực thi `myls` sẽ được tạo sau khi biên dịch thành công.

### 5.2. Biên dịch với cảnh báo nghiêm ngặt

```sh
make clean
make CFLAGS='-Wall -Wextra -Werror -std=c11 -Iinclude'
```

Tùy chọn `-Werror` yêu cầu quá trình biên dịch không có cảnh báo từ các nhóm được bật.

### 5.3. Chạy và dọn dẹp

```sh
./myls
./myls -la
./myls -lR src
make clean
```

Lệnh `make clean` xóa file thực thi và các file object, không xóa mã nguồn.

## 6. Ví dụ sử dụng

| Lệnh | Mục đích |
| --- | --- |
| `./myls` | Liệt kê thư mục hiện tại |
| `./myls -a` | Liệt kê cả các mục ẩn |
| `./myls -l` | Hiển thị thông tin chi tiết |
| `./myls -lh` | Hiển thị thông tin chi tiết với kích thước dễ đọc |
| `./myls -il src` | Hiển thị inode và thông tin chi tiết trong `src` |
| `./myls -S src` | Sắp xếp theo kích thước |
| `./myls -t src` | Sắp xếp theo thời gian |
| `./myls -R src` | Duyệt đệ quy thư mục `src` |
| `./myls -lR src` | Hiển thị chi tiết khi duyệt đệ quy |
| `./myls -d src` | Hiển thị bản thân thư mục `src` |
| `./myls -ilsh src` | Kết hợp inode, số khối, đơn vị dễ đọc và thông tin chi tiết |
| `./myls README.md Makefile src` | Xử lý nhiều đối số đồng thời |

## 7. Thiết kế và triển khai

### 7.1. Phân tích đối số

Module `options.c` chuyển các tùy chọn được nhập thành trạng thái cấu hình của chương trình. `main.c` tập hợp các đối số hợp lệ, phân biệt đối số tập tin với thư mục và điều phối thứ tự hiển thị.

### 7.2. Làm việc với hệ thống tập tin

Chương trình sử dụng các API như `opendir()`, `readdir()`, `closedir()`, `lstat()`, `stat()` và `readlink()` để duyệt các mục và lấy metadata. Những trường hợp đặc biệt như symbolic link, đường dẫn không tồn tại và lỗi quyền truy cập được xử lý riêng.

### 7.3. Quản lý dữ liệu và sắp xếp

Danh sách các mục được quản lý bằng vùng nhớ cấp phát động. Module `sorting.c` sử dụng thuật toán Heap Sort và các phép so sánh theo tên, kích thước hoặc thời gian; có hỗ trợ đảo thứ tự và bỏ sắp xếp theo tùy chọn.

### 7.4. Duyệt đệ quy

Với `-R`, chương trình duyệt nội dung của các thư mục con theo cấu trúc phân cấp, đồng thời xử lý việc hiển thị tiêu đề thư mục và các quy tắc đối với tên ẩn.

### 7.5. Định dạng hiển thị

Module `display.c` xây dựng các cột thông tin trong chế độ chi tiết: quyền, số hard link, chủ sở hữu, nhóm, kích thước, thời gian và tên. Chương trình còn tính độ rộng cột dùng chung để căn chỉnh giữa các phần tử, hỗ trợ inode, số khối và các số thiết bị `major, minor`.

## 8. Kiểm thử

### 8.1. Phương pháp

Kết quả của `myls` được so sánh trực tiếp với lệnh `ls` trên NetBSD 10.1 bằng cách chuyển hướng kết quả ra tập tin và chạy `diff -u`. Với chế độ hiển thị thường, tùy chọn `-1` được thêm vào lệnh `ls` tham chiếu để so sánh cùng dạng một mục trên mỗi dòng.

Ví dụ:

```sh
./myls -lR src > /tmp/myls-output.txt
ls -lR src > /tmp/netbsd-ls-output.txt
diff -u /tmp/netbsd-ls-output.txt /tmp/myls-output.txt
```

Khi `diff` không in khác biệt và mã thoát bằng `0`, hai đầu ra khớp ở lần kiểm thử đó.

### 8.2. Kết quả đã thực hiện

| Nhóm kiểm thử | Kết quả |
| --- | --- |
| 19 tùy chọn chạy độc lập | **19/19 đạt** trong bộ kiểm thử hồi quy |
| Tổ hợp tùy chọn | Các tổ hợp đã kiểm tra đạt sau khi xử lý dữ liệu thời gian ổn định |
| Đệ quy và thư mục rỗng | Khớp trong các tình huống đã kiểm tra |
| Symbolic link thường và bị hỏng | Khớp trong các tình huống đã kiểm tra |
| Thiết bị đặc biệt trong `/dev` | Khớp với các tổ hợp đã kiểm tra |
| Căn chỉnh inode, số khối và cột chi tiết | Khớp trong các tình huống đã kiểm tra |
| Nhiều đối số tập tin/thư mục | Khớp trong các tình huống đã kiểm tra |
| Đường dẫn không tồn tại và lỗi quyền truy cập | Khớp đầu ra, thông báo lỗi (sau chuẩn hóa tên chương trình) và mã thoát |
| Biên dịch sạch với `-Werror` | **Thành công** |

Một số metadata như *atime* hoặc kích thước thư mục cha có thể thay đổi khi hệ thống đang hoạt động. Các trường hợp này cần kiểm thử bằng dữ liệu ổn định trước khi kết luận có lỗi xử lý.

## 9. Phạm vi và giới hạn

Đây là bài triển khai phục vụ học tập theo phạm vi 19 tùy chọn của đề bài, không nhằm thay thế hoàn toàn tiện ích `ls(1)` của hệ điều hành.

Chương trình hiển thị đầu ra thông thường theo dạng một mục trên mỗi dòng. Kết quả kiểm thử nêu trên phản ánh các trường hợp thực tế đã thực hiện và không bảo đảm tính tương đương tuyệt đối với mọi đầu vào hoặc môi trường.

## 10. Kết luận

Dự án đã triển khai chương trình `myls` theo kiến trúc 7 module C, hỗ trợ 19 tùy chọn trong phạm vi đề tài, và đối chiếu nhiều tình huống thực tế với lệnh `ls(1)` trên NetBSD 10.1. Quá trình thực hiện giúp củng cố kỹ năng thao tác API hệ thống tập tin, tổ chức mã nguồn, quản lý bộ nhớ động, định dạng kết quả và kiểm thử hồi quy.

## 11. Tài liệu tham khảo

1. NetBSD 10.1 – tài liệu hướng dẫn `ls(1)` được cung cấp trong đề bài.
2. NetBSD Manual Pages – `stat(2)`, `lstat(2)`, `readlink(2)`, `opendir(3)`, `readdir(3)` và các API liên quan.
3. Tài liệu học phần Lập trình hệ thống.

---

**Sinh viên thực hiện:** Mai Văn Lương – **MSSV:** 24IT154  
**Repository dự kiến:** `MaiVanLuong_24IT154_midterm` (chỉ cập nhật liên kết sau khi tạo và đẩy lên GitHub).
