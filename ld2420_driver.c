#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "ld2420"

// Biến lưu số major của thiết bị ký tự, dùng để tạo /dev/ld2420
static int major;
// Con trỏ tới lớp thiết bị, tạo /sys/class/ld2420. Tên lớp dựa trên DEVICE_NAME
static struct class *ld_class;
// Con trỏ tới thiết bị, tạo /dev/ld2420 để người dùng tương tác
static struct device *ld_device;

// Hàm đọc từ /dev/ld2420, hiện chưa hỗ trợ
static ssize_t ld_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    // Trả về -EINVAL do chưa hỗ trợ. Thay đổi: Thêm logic đọc dữ liệu cảm biến LD2420 qua UART
    return -EINVAL; // Không hỗ trợ read, để user làm trực tiếp
}

// Hàm ghi vào /dev/ld2420, hiện chưa hỗ trợ
static ssize_t ld_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    // Trả về -EINVAL do chưa hỗ trợ. Thay đổi: Thêm logic gửi lệnh cấu hình (khoảng cách, độ nhạy) qua UART
    return -EINVAL; // Không hỗ trợ write, để user làm trực tiếp
}

// Hàm xử lý IOCTL để cấu hình thiết bị, hiện chưa hỗ trợ
static long ld_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    // Trả về -ENOTTY do chưa hỗ trợ. Thay đổi: Định nghĩa lệnh IOCTL để cấu hình khoảng cách, độ nhạy
    return -ENOTTY; // Không hỗ trợ ioctl, user làm trực tiếp
}

// Cấu trúc định nghĩa các hàm xử lý cho thiết bị ký tự
static struct file_operations fops = {
    // Chủ sở hữu là mô-đun hiện tại, đảm bảo liên kết với kernel
    .owner = THIS_MODULE,
    // Gán hàm đọc cho thiết bị
    .read = ld_read,
    // Gán hàm ghi cho thiết bị
    .write = ld_write,
    // Gán hàm IOCTL cho thiết bị
    .unlocked_ioctl = ld_ioctl,
};

// Hàm khởi tạo mô-đun kernel
static int __init ld_init(void) {
    // Đăng ký thiết bị ký tự với số major động (0), trả về số major được cấp
    major = register_chrdev(0, DEVICE_NAME, &fops);
    // Tạo lớp thiết bị trong /sys/class/ld2420, dùng DEVICE_NAME làm tên
    ld_class = class_create(THIS_MODULE, DEVICE_NAME);
    // Tạo thiết bị trong /dev/ld2420 với số major và minor (0)
    ld_device = device_create(ld_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    // In thông báo khởi tạo thành công vào log kernel
    printk(KERN_INFO "[LD2420] Simple character device loaded\n");
    // Trả về 0 để báo khởi tạo thành công
    return 0;
}


static void __exit ld_exit(void) {
    // Xóa thiết bị /dev/ld2420 khỏi hệ thống
    device_destroy(ld_class, MKDEV(major, 0));
    // Xóa lớp thiết bị /sys/class/ld2420
    class_destroy(ld_class);
    // Hủy đăng ký thiết bị ký tự với số major và tên DEVICE_NAME
    unregister_chrdev(major, DEVICE_NAME);
    // In thông báo gỡ mô-đun vào log kernel
    printk(KERN_INFO "[LD2420] Device unloaded\n");
}

module_init(ld_init);
module_exit(ld_exit);

MODULE_LICENSE("GPL");