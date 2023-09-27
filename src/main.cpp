#include <Arduino.h>
#include <ledRunning/src/ledRunning.h>
// định nghĩa các chân điều khiển led
#define LED_GREEN 3
#define LED_BLUE 4
#define LED_RED 5
// định nghĩa các trạng thái chân tín hiệu nút nhấn và chính nó
#define PRESSING 0          // khi nút nhấn được nhấn tín hiệu đọc được là 0 (voltage) tương đương với mức LOW của arduino và logic 0
#define NONE_PRESSING 1     // khi nút nhấn được nhấn tín hiệu đọc được là giá trị điện áp =0.6*VCC =0.6*5~ 3 (voltage) tương đương với mức HIGH của arduino và logic 1
#define SWITCH_BUTTON_PIN 2 // dùng chân số 2 để nhận tín hiệu nút nhấn
// định nghĩa các sự kiện của nút nhấn được tác động bởi user
// tại sao phải làm như vầy? là tại vì sử dụng switch để so sánh điều kiện sẽ nhanh hơn sử dụng if nhưng switch không so sánh string được
// nên định nghĩa tên cho dễ hiểu và gắn nó giá trị là 1 con số thi có thể sử dụng switch (tại dòng code 80)
#define FREE_STATE 0         // Không nhấn
#define FIRST_CLICK_STATE 1  // nhấn lần đầu tiên
#define CLICK_STATE 2        // đúng là đã nhấn 1 lần
#define DOUBLE_CLICK_STATE 3 // nhấn double click
#define HOLD_STATE 4         // nhấn giữ

// Trong những điều kiện môi trường khắc nghiệt hoặc code ngu thì vi điều khiển (vđk||mcu) sẽ bị treo không rõ nguyên nhân vì vậy để xác định rằng khi mcu còn đang chạy
// chúng ta cho nó chớp tắc cái đèn nào đó và mcu bị treo sẽ không chớp tắc
// đây cũng là 1 ví dụ nho nhỏ về lập trình hướng đối tượng và xây dựng module hóa (thư viện)
// file này đang nằm trong thư mục src/ledRunning/src/ledRunning.h

// khai báo biến ledRunMCu là một đối tượng LedRunning
LedRunning ledRunMCU;

// khai báo các biến cần thiết cho quá trình quét tín hiệu nút nhấn
unsigned long pressed_timeStamp_Start;    // thời điểm ghi nhận tín hiệu nút được nhấn xuống
bool wasPressed;                          // biến ghi nhận đã được nhấn xuống dùng để so sánh thời gian và tránh vòng lặp liên tục
unsigned long last_first_click_timeStamp; // thời điểm ghi nhận trạng thái nút nhấn đầu tiên dùng để so sánh tiếp diễn trạng thái double click

// trong hàm loop chúng ta không nên gọi hàm millis() quá nhiều sẽ ảnh hưởng đến hiệu năng của mcu cũng như so sánh sẽ bất đồng bộ vì tại mỗi thời điểm gọi hàm millis()
// là một mốc thời gian tiếp diễn nên giá trị trả về sẽ khác nhau
unsigned long timeLine_now;
// thời gian chờ sau khi nút được nhấn lần đầu tiên dùng để chờ nếu nút được nhấn 1 lần nữa thì là double click ,nếu không
// nhấn thì là click
unsigned long doubleClick_timeStamp_delta = 100;
// thời gian quy định việc giữ nút nhấn bao lâu thì sẽ quy định là trạng thái nhấn giữ
unsigned long hold_timeStamp_delta = 2000;
uint8_t BUTTON_STATE = FREE_STATE;

void setup()
{
  Serial.begin(115200); // bật Serial để debug khi có lỗi
  // cấu hình các đèn
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(SWITCH_BUTTON_PIN, INPUT_PULLUP);
  // cấu hình cho đối tượng LedRunning
  ledRunMCU.initialize(LED_BUILTIN);
  Serial.println("notification: System started if system is running,led bulit-in will be flash every 200 milliseconds");
}

void loop()
{
  ledRunMCU.loop(1000);                                                        // yên cầu đối tượng LedRunning nhấp nháy mỗi 1 giây để biêt rằng mcu không bị treo
  timeLine_now = millis();                                                     // lấy giá trị thời gian hiện tại 1 lần cho 1 lần loop để tránh gọi hàm millis() quá nhiều làm yếu hiệu năng mcu
  if (BUTTON_STATE == FREE_STATE)                                              // nếu nút nhấn đang ở trạng thái tự do không được nhấn cũng như đang không có đang chờ sự kiện double click
  {                                                                            // thì
    if (digitalRead(SWITCH_BUTTON_PIN) == PRESSING && wasPressed == false)     // nếu nút đang được nhấn và trước đó chưa có ghi nhận là nút được nhấn ==>(*1)
    {                                                                          // thì
      pressed_timeStamp_Start = timeLine_now;                                  // gắn thời gian nhấn nút bằng thời gian hiện tại
      Serial.println("warning: detects button has been pressed!");             // debug to console
      wasPressed = true;                                                       // ghi nhận rằng nút đươc nhấn rồi
    }                                                                          // vòng lặp của hàm loop() sẽ ngừng so sánh (*1) cho đến khi thỏa 2 điều kiện là nút không được nhấn nữa và trạng thái đã được nhấn bị reset về false
    else if (digitalRead(SWITCH_BUTTON_PIN) == PRESSING && wasPressed == true) // ngược lại của (*1) nếu nút đang được nhấn và trạng thái đã được nhấn được ghi nhận(tức là đang nhấn giữ chưa có nhả nút ra)==>(*2)
    {
      if ((timeLine_now - pressed_timeStamp_Start) >= hold_timeStamp_delta) // thì nếu thời gian hiện tại trừ cho thời gian ghi nhận từ lúc bắt đầu nhấn lớn hơn hoặc bằng thời gian quy định trạng thái nhấn giữ
      {
        BUTTON_STATE = HOLD_STATE;                            // thì ghi nhận sự kiện cho nút nhấn là HOLD_STATE
        Serial.println("warning: button is being held down"); // debug to console
      }
    }
    else if (digitalRead(SWITCH_BUTTON_PIN) == NONE_PRESSING && wasPressed == true) // ngươc lại điều khiện của (*1) và (*2) nếu nút được thả ra và trước đó có ghi nhận đã đươc nhấn (thả nút sau khi nhấn)==>(*3)
    {
      // TẠI SAO PHẢI DÙNG biến wasPress??? nếu không có nó thì điều kiện (*2) luôn thỏa đáng khi chả có ai nhấn nút tín hiệu giá trị của chân tín hiệu nút nhấn mỗi vòng loop là liên tục =1(NONE_PRESSING)
      // như thế sẽ sinh lỗi còn lỗi gì thì xóa "&& wasPressed == true" của (*2) đi rồi chạy thử mà xem
      if ((timeLine_now - pressed_timeStamp_Start) < hold_timeStamp_delta) // nếu thời gian hiện tại trừ cho thời gian ghi nhận sự kiện nhấn nút trước đó nhỏ hơn thời gian chờ sự kiện doule click
      {                                                                    // thì
        BUTTON_STATE = FIRST_CLICK_STATE;                                  // mới tính là nút được nhấn lần đầu tiên ghi nhận sự kiện FIRST_CLICK_STATE
        last_first_click_timeStamp = timeLine_now;                         // ghi nhận thời gian nhả nút để so sánh trong khi chờ lần nhấn tiếp theo xem nó có đúng là double click hay không
        Serial.println("warning: button has been first clicked");          // debug to console
      }
      pressed_timeStamp_Start = millis(); // reset lại thời gian ghi nhận nút được nhấn vì tất nhiên là bây giừ nút đã đươc nhã ra rồi
      wasPressed = false;                 // reset lại trạng thái ghi nhận nút đã và đang nhấn
    }
  }
  else // sau khi nút nhấn được ghi nhận các sự kiện
  {
    switch (BUTTON_STATE)
    {
    case FIRST_CLICK_STATE:                                                           // khi xảy ra sự kiên nhấn lần đầu tiên
      if ((timeLine_now - last_first_click_timeStamp) <= doubleClick_timeStamp_delta) // nếu khoảng thời gian xảy ra 2 lần nhấn nút nhỏ hơn hoặc bằng thòi gian chờ ghi nhận sự kiện double click
      {
        if (digitalRead(SWITCH_BUTTON_PIN) == PRESSING) // nếu nút còn đang nhấn thì gán cho nó sự kiện DOUBLE_CLICK_STATE (*4)
        {
          BUTTON_STATE = DOUBLE_CLICK_STATE;
          Serial.println("warning: button has been double clicked");
        }
      }
      else // ngược lại điều kiện (*4) nếu nút không được nhấn trong thời gian doubleClick_timeStamp_delta
      {
        BUTTON_STATE = CLICK_STATE; // đó là 1 lần nhấn => sinh ra sự kiện click
        Serial.println("warning: button has been true clicked");
      }
      break;
    case CLICK_STATE: // sự kiện click

      if (digitalRead(SWITCH_BUTTON_PIN) == NONE_PRESSING) // nếu phím nhả ra rồi thì nên reset sự kiện nút nhấn về tự do để còn loop so sánh tiếp
      {
        BUTTON_STATE = FREE_STATE;
      }

      break;
    case DOUBLE_CLICK_STATE:                               // sự kiện double click
      if (digitalRead(SWITCH_BUTTON_PIN) == NONE_PRESSING) // nếu phím nhả ra rồi thì nên reset sự kiện nút nhấn về tự do để còn loop so sánh tiếp
      {
        BUTTON_STATE = FREE_STATE;
      }
      break;
    case HOLD_STATE:                                       // sự kiện nhấn giữ
      if (digitalRead(SWITCH_BUTTON_PIN) == NONE_PRESSING) // nếu phím nhả ra rồi thì nên reset sự kiện nút nhấn về tự do để còn loop so sánh tiếp
      {
        BUTTON_STATE = FREE_STATE;
      }
      break;
    default:
      break;
    }
  }
}
