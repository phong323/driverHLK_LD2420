#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
// ICANON( Chế độ nhập dòng): chờ người dùng nhấn Enter mới xử lý.
// ECHO: Hiển thị ký tự người dùng nhập 
// ECHOE: Khi nhấn Backspace, sẽ xóa ký tự trước đó trên màn hình.
// ISIG Cho phép các tín hiệu như Ctrl+C , Ctrl+Z
#define UART_DEVICE "/dev/ttyS0"
// Đọc cấu hình hiện tại của UART và lưu vào tty
int configure_uart(int fd) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr failed");
        return -1;
    }
//Đặt tốc độ truyền và nhận là 115200 baud
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag &= ~PARENB; // không dùng parity
    tty.c_cflag &= ~CSTOPB; // 1 stop bit
    tty.c_cflag &= ~CSIZE;  
    tty.c_cflag |= CS8; // 8 bit dữ liệu
    tty.c_cflag |= CREAD | CLOCAL; // bật chế độ đọc và tắt điều khiển 
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // tắt ICANON, ECHO, ECHOE, ISIG
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // tắt IXON, IXOFF, IXANY
    tty.c_oflag &= ~OPOST;// tắt dữ liệu đầu ra

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10; // time out sau 1 giây k có dữ liệu
// áp dụng cấu hình mới
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr failed");
        return -1;
    }

    return 0;
}
// gửi gói lệnh qua UART tơis module radar
void send_command(int fd, const unsigned char *cmd, size_t len) {
    write(fd, cmd, len);
    usleep(100000); // cho 100ms de thiet bi su ly
}
// doc du lieu UART vao buffer 
void monitor_data(int fd) {
    unsigned char buf[256];
    ssize_t len;

    printf("Bắt đầu theo dõi trạng thái từ LD2420...\n");
    while (1) {
        len = read(fd, buf, sizeof(buf));
        // tim goi du lieu bat dau bang AA FF 00
        if (len >= 5) {
            for (ssize_t i = 0; i < len - 4; i++) {
                if (buf[i] == 0xAA && buf[i + 1] == 0xFF && buf[i + 2] == 0x00) {
                    unsigned char status = buf[i + 3];
                    if (status == 0x00)
                        printf("-> Không có ai\n");
                    else if (status == 0x01)
                        printf("-> Có người đứng yên\n");
                    else if (status == 0x02)
                        printf("-> Có người di chuyển\n");
                    else
                        printf("-> Trạng thái không xác định: %02X\n", status);
                }
            }
        }
        usleep(100000);
    }
}
// mở UART ở chế độ đọc/ghi ko điều khiển bằng terminalterminal
int main() {
    int fd = open(UART_DEVICE, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("Failed to open UART");
        return 1;
    }
// cấu hình cho UART và thoát nếu thất bại
    if (configure_uart(fd) != 0) {
        close(fd);
        return 1;
    }

    // Cấu hình cảm biến
    unsigned char enter_config[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0x00, 0x00, 0xF8};
    send_command(fd, enter_config, sizeof(enter_config));
// khoảng cách phát hiện của cảm biến
    unsigned char set_distance[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x0A, 0x01, 0x01, 0x01, 0x05, 0x00, 0x00, 0xE9};
    send_command(fd, set_distance, sizeof(set_distance));
// độ nhạy của cảm biến
    unsigned char set_sensitivity[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x03, 0x03, 0xB4, 0x00, 0x3E};
    send_command(fd, set_sensitivity, sizeof(set_sensitivity));
// thoát khỏi chế độ cấu hình
    unsigned char exit_config[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0x01, 0x00, 0xF9};
    send_command(fd, exit_config, sizeof(exit_config));

    // Bắt đầu theo dõi
    monitor_data(fd);

    close(fd);
    return 0;
}
