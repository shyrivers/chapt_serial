#include <chrono>
#include "rclcpp/rclcpp.hpp"
#include "serial_driver/serial_driver.hpp"

using namespace std::chrono_literals;

class SerialTestNode : public rclcpp::Node {
public:
  SerialTestNode() : Node("serial_test_node") {
    // 初始化串口 (端口名根据实际情况修改)
    io_context_ = std::make_shared<IoContext>();
    serial_driver_ = std::make_unique<drivers::serial_driver::SerialDriver>(*io_context_);
    try {
      serial_driver_->init_port("/dev/ttyUSB0",
        drivers::serial_driver::SerialPortConfig(
        115200, 
        drivers::serial_driver::FlowControl::NONE,
        drivers::serial_driver::Parity::NONE,
        drivers::serial_driver::StopBits::ONE
      ));
      // 通过 SerialPort 对象操作串口
      auto port = serial_driver_->port();
      if (!port) {
        RCLCPP_ERROR(get_logger(), "串口初始化失败");
        rclcpp::shutdown();
        return;
      }
      port->open();  // 正确的 open() 方式
      RCLCPP_INFO(get_logger(), "串口已开启");
      // 定时发送 'A' 并等待回复
      // 在回调函数中优化未使用的变量

    timer_ = this->create_wall_timer(
    1000ms, [this, port]() {
        const std::string tx_msg ="CUG_serial_test";
        std::vector<uint8_t> tx_buffer(tx_msg.begin(),tx_msg.end());  // 发送'CUG_serial_test'
        
        // 实际使用 sent_bytes
        size_t sent_bytes = port->send(tx_buffer);
        RCLCPP_INFO(this->get_logger(), "Sent '%s' (%zu bytes)", tx_msg.c_str(),sent_bytes); // 打印发送字节数

         // 2. 接收STM32返回的 "CUG_serial_received"
        std::vector<uint8_t> rx_buffer(64);// 预分配64字节缓冲区
        size_t received_bytes = port->receive(rx_buffer);
        if (received_bytes > 0) {
            std::string rx_msg(rx_buffer.begin(), rx_buffer.begin() + received_bytes);
            RCLCPP_INFO(this->get_logger(), "Received: '%s'", rx_msg.c_str());
          } else {
            RCLCPP_WARN(this->get_logger(), "No data received");
          }


    });

    } catch (const std::exception &ex) {
      RCLCPP_ERROR(get_logger(), "串口错误: %s", ex.what());
      rclcpp::shutdown();
    }
  }
private:
  std::shared_ptr<IoContext> io_context_;  // 必须保持 io_context_ 存活
  std::unique_ptr<drivers::serial_driver::SerialDriver> serial_driver_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<SerialTestNode>());
  rclcpp::shutdown();
  return 0;
}