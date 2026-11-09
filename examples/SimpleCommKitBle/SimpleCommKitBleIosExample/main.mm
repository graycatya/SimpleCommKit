#import <UIKit/UIKit.h>
#include <SimpleCommKitBleCentral.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>
using namespace SimpleCommKit;
namespace {
std::mutex logMutex;
std::function<void(const std::string&)> logSink;
std::ofstream logFile;
class ProbeMessage {
    std::ostringstream text;
public:
    ~ProbeMessage() {
        const auto message = text.str();
        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile) { logFile << message; logFile.flush(); }
        if (logSink) logSink(message);
        fprintf(stderr, "%s", message.c_str());
    }
    template<class T> ProbeMessage& operator<<(const T& value) { text << value; return *this; }
    ProbeMessage& operator<<(std::ostream& (*fn)(std::ostream&)) { fn(text); return *this; }
};
ProbeMessage consoleOut() { return {}; }
ProbeMessage consoleError() { return {}; }
struct Session {
    std::shared_ptr<SimpleCommKitBleCentral> central = std::make_shared<SimpleCommKitBleCentral>();
    std::atomic<bool> stop{false};
    std::atomic<int> phase{0}; // 0 idle, 1 initialization/scan, 2 IM948 session
    std::atomic<ErrorCode> error{0};
};
}
#include "Im948ReadProbe.hpp"

@interface ProbeController : UIViewController <UITableViewDataSource, UITableViewDelegate> {
    std::shared_ptr<Session> _session;
    std::vector<SimpleCommKitBlePeripheral> _devices;
    dispatch_queue_t _worker;
    NSInteger _selected;
    BOOL _busy;
    UIBackgroundTaskIdentifier _backgroundTask;
}
@property(nonatomic, strong) UILabel* status;
@property(nonatomic, strong) UIButton* scan;
@property(nonatomic, strong) UIButton* test;
@property(nonatomic, strong) UIButton* stop;
@property(nonatomic, strong) UITableView* table;
@property(nonatomic, strong) UITextView* log;
- (void)appendLog:(NSString*)message;
- (void)scanDevices;
- (void)runTests;
- (void)requestStop;
- (void)enterBackground;
@end

@implementation ProbeController
- (UIButton*)button:(NSString*)title selector:(SEL)selector {
    UIButton* button = [UIButton buttonWithType:UIButtonTypeSystem];
    [button setTitle:title forState:UIControlStateNormal];
    [button addTarget:self action:selector forControlEvents:UIControlEventTouchUpInside];
    return button;
}
- (void)viewDidLoad {
    [super viewDidLoad];
    _selected = NSNotFound; _backgroundTask = UIBackgroundTaskInvalid;
    _worker = dispatch_queue_create("org.simplecommkit.ble.ios.probe", DISPATCH_QUEUE_SERIAL);
    self.view.backgroundColor = UIColor.systemBackgroundColor;
    self.status = [UILabel new]; self.status.text = @"就绪：点击扫描申请蓝牙权限";
    self.status.numberOfLines = 0; self.status.accessibilityIdentifier = @"probe.status";
    self.scan = [self button:@"扫描 25 秒" selector:@selector(scanDevices)];
    self.test = [self button:@"运行 IM948 两轮测试" selector:@selector(runTests)];
    self.stop = [self button:@"停止（先完成当前参数恢复）" selector:@selector(requestStop)];
    self.scan.accessibilityIdentifier = @"probe.scan"; self.test.accessibilityIdentifier = @"probe.test";
    self.stop.accessibilityIdentifier = @"probe.stop"; self.test.enabled = NO; self.stop.enabled = NO;
    UILabel* hint = [UILabel new]; hint.numberOfLines = 0;
    hint.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    hint.text = @"选择设备后测试连接、读取及通知。测试会临时订阅传感器字段，结束前恢复并核对原参数。请保持 App 在前台。";
    self.table = [[UITableView alloc] initWithFrame:CGRectZero style:UITableViewStylePlain];
    self.table.dataSource = self; self.table.delegate = self;
    self.table.accessibilityIdentifier = @"probe.devices";
    self.log = [UITextView new]; self.log.editable = NO;
    self.log.font = [UIFont monospacedSystemFontOfSize:11 weight:UIFontWeightRegular];
    self.log.accessibilityIdentifier = @"probe.log";
    UIStackView* stack = [[UIStackView alloc] initWithArrangedSubviews:@[self.status, self.scan, self.test, self.stop, hint, self.table, self.log]];
    stack.axis = UILayoutConstraintAxisVertical; stack.spacing = 6; stack.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:stack];
    UILayoutGuide* safe = self.view.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [stack.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:12],
        [stack.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-12],
        [stack.topAnchor constraintEqualToAnchor:safe.topAnchor constant:8],
        [stack.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-8],
        [self.table.heightAnchor constraintEqualToAnchor:self.view.heightAnchor multiplier:0.27],
        [self.log.heightAnchor constraintGreaterThanOrEqualToConstant:80]]];
    NSString* documents = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    __weak ProbeController* weakSelf = self;
    { std::lock_guard<std::mutex> lock(logMutex);
      logFile.open([[documents stringByAppendingPathComponent:@"BleProbe.log"] fileSystemRepresentation], std::ios::trunc);
      logSink = [weakSelf](const std::string& message) {
          NSString* text = [[NSString alloc] initWithBytes:message.data() length:message.size() encoding:NSUTF8StringEncoding];
          [weakSelf appendLog:text ?: @"日志编码错误\n"];
      }; }
    consoleOut() << "[IOS READY] Foreground UI loaded; Bluetooth is initialized only on scan.\n";
    NSArray<NSString*>* args = NSProcessInfo.processInfo.arguments;
    if ([args containsObject:@"--scan-only"] || [args containsObject:@"--im948-test"])
        dispatch_async(dispatch_get_main_queue(), ^{ [weakSelf scanDevices]; });
}
- (void)appendLog:(NSString*)message {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSString* content = [self.log.text stringByAppendingString:message];
        if (content.length > 80000) content = [content substringFromIndex:content.length - 60000];
        self.log.text = content;
        if (content.length) [self.log scrollRangeToVisible:NSMakeRange(content.length - 1, 1)];
    });
}
- (void)setBusy:(BOOL)busy text:(NSString*)text {
    _busy = busy; self.status.text = text; self.scan.enabled = !busy;
    self.test.enabled = !busy && _selected != NSNotFound; self.stop.enabled = busy;
    self.table.userInteractionEnabled = !busy;
    if (!busy && _backgroundTask != UIBackgroundTaskInvalid) {
        [UIApplication.sharedApplication endBackgroundTask:_backgroundTask]; _backgroundTask = UIBackgroundTaskInvalid;
    }
}
- (void)scanDevices {
    if (_busy || UIApplication.sharedApplication.applicationState == UIApplicationStateBackground) return;
    // Each run gets a new session; previous delegates and callbacks are retired.
    _session = std::make_shared<Session>(); auto session = _session;
    _devices.clear(); _selected = NSNotFound; [self.table reloadData];
    [self setBusy:YES text:@"扫描中；首次使用请允许蓝牙访问…"];
    __weak ProbeController* weakSelf = self;
    session->phase = 1;
    dispatch_async(_worker, ^{
        auto& central = *session->central;
        central.set_Callback_Error([weak = std::weak_ptr<Session>(session)](auto code) {
            if (auto state = weak.lock()) state->error = code;
            consoleError() << "[IOS BLE ERROR] " << code << " " << SimpleCommKitErrorMap::GetErrorDescription(code) << '\n';
        });
        central.adapter_Set_Callback_On_Scan_Start([] { consoleOut() << "[IOS SCAN START]\n"; });
        central.adapter_Set_Callback_On_Scan_Stop([] { consoleOut() << "[IOS SCAN STOP]\n"; });
        auto adapters = central.get_Adapters();
        if (!adapters.empty() && !session->stop) {
            central.set_CurrentAdapter(adapters.front());
            if (!session->stop) central.adapter_Scan_For(25000);
        }
        central.adapter_Scan_Stop();
        auto devices = central.adapter_Get_Scan_Results();
        for (const auto& device : devices)
            consoleOut() << "[IOS DEVICE] " << device.identifier << " (" << device.address << ") RSSI=" << device.rssi << '\n';
        consoleOut() << "[IOS SCAN RESULT] devices=" << devices.size() << " error=" << session->error.load() << '\n';
        session->phase = 0;
        dispatch_async(dispatch_get_main_queue(), ^{
            ProbeController* controller = weakSelf; if (!controller) return;
            controller->_devices = devices; [controller.table reloadData];
            const BOOL stopped = session->stop.load();
            NSString* text = session->error ? @"扫描未完成，请查看错误日志和蓝牙权限" :
                [NSString stringWithFormat:@"%@ %lu 个设备，请点选 IM948", stopped ? @"已停止，发现" : @"发现", (unsigned long)devices.size()];
            [controller setBusy:NO text:text];
            if (!stopped && !session->error && [NSProcessInfo.processInfo.arguments containsObject:@"--im948-test"] &&
                UIApplication.sharedApplication.applicationState == UIApplicationStateActive) {
                NSInteger match = NSNotFound; unsigned matches = 0;
                for (size_t i = 0; i < devices.size(); ++i) {
                    auto name = devices[i].identifier;
                    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
                    if (name.find("im948") != std::string::npos) { match = i; ++matches; }
                }
                if (matches == 1) { controller->_selected = match; [controller runTests]; }
                else consoleError() << "[IOS TEST SKIP] Need exactly one advertised IM948, found " << matches << '\n';
            }
        });
    });
}
- (void)runTests {
    if (_busy || _selected == NSNotFound || !_session ||
        UIApplication.sharedApplication.applicationState != UIApplicationStateActive) return;
    auto session = _session; auto device = _devices.at(_selected);
    session->stop = false; session->phase = 2;
    [self setBusy:YES text:@"IM948 测试中：请保持前台，结束时恢复参数"];
    __weak ProbeController* weakSelf = self;
    dispatch_async(_worker, ^{
        bool passed = false;
        try {
            auto& central = *session->central;
            consoleOut() << "[IOS TEST TARGET] " << device.identifier << " " << device.address << '\n';
            central.set_CurrentPeripheral(device);
            if (!session->stop) central.peripheral_Connect();
            if (central.peripheral_Is_Connected() && !session->stop) {
                auto services = central.peripheral_Services();
                passed = testIm948(central, services, [session] { return session->stop.load(); });
            }
            if (central.peripheral_Is_Connected()) central.peripheral_Disconnect();
        } catch (const std::exception& error) { consoleError() << "[IOS TEST EXCEPTION] " << error.what() << '\n'; }
        session->phase = 0;
        consoleOut() << (passed ? "[IOS IM948 PASS]\n" : "[IOS IM948 INCOMPLETE]\n");
        dispatch_async(dispatch_get_main_queue(), ^{
            [weakSelf setBusy:NO text:passed ? @"两轮测试通过，已恢复参数并断开" : @"测试未完成，请查看日志中的恢复结果"];
        });
    });
}
- (void)requestStop {
    if (!_session || !_busy) return;
    auto session = _session; session->stop = true;
    self.status.text = @"正在停止；传感器会话先完成当前参数恢复…";
    if (session->phase == 1) dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
        session->central->adapter_Scan_Stop();
    });
}
- (void)enterBackground {
    if (!_busy) return;
    __weak ProbeController* weakSelf = self;
    auto session = _session;
    _backgroundTask = [UIApplication.sharedApplication beginBackgroundTaskWithName:@"BLE cleanup" expirationHandler:^{
        consoleError() << "[IOS BACKGROUND EXPIRED] Cancelling connection; parameter restoration may be incomplete.\n";
        session->stop = true;
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{ session->central->peripheral_Disconnect(); });
        ProbeController* controller = weakSelf;
        if (controller && controller->_backgroundTask != UIBackgroundTaskInvalid) {
            [UIApplication.sharedApplication endBackgroundTask:controller->_backgroundTask];
            controller->_backgroundTask = UIBackgroundTaskInvalid;
        }
    }];
    [self requestStop];
}
- (NSInteger)tableView:(UITableView*)tableView numberOfRowsInSection:(NSInteger)section {
    (void)tableView; (void)section; return _devices.size();
}
- (UITableViewCell*)tableView:(UITableView*)tableView cellForRowAtIndexPath:(NSIndexPath*)indexPath {
    UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:@"device"];
    if (!cell) cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"device"];
    const auto& device = _devices.at(indexPath.row);
    cell.textLabel.text = device.identifier.empty() ? @"未提供名称" : [NSString stringWithUTF8String:device.identifier.c_str()];
    cell.detailTextLabel.text = [NSString stringWithFormat:@"%s · RSSI %d", device.address.c_str(), device.rssi];
    cell.detailTextLabel.adjustsFontSizeToFitWidth = YES;
    cell.accessoryType = indexPath.row == _selected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    return cell;
}
- (void)tableView:(UITableView*)tableView didSelectRowAtIndexPath:(NSIndexPath*)indexPath {
    _selected = indexPath.row; self.test.enabled = !_busy; [tableView reloadData];
}
@end

@interface ProbeApp : UIResponder <UIApplicationDelegate>
@property(nonatomic, strong) UIWindow* window;
@property(nonatomic, strong) ProbeController* probe;
@end
@implementation ProbeApp
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)options {
    (void)application; (void)options;
    self.probe = [ProbeController new]; self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    self.window.rootViewController = self.probe; [self.window makeKeyAndVisible]; return YES;
}
- (void)applicationDidEnterBackground:(UIApplication*)application { (void)application; [self.probe enterBackground]; }
@end
int main(int argc, char** argv) { @autoreleasepool { return UIApplicationMain(argc, argv, nil, NSStringFromClass(ProbeApp.class)); } }
