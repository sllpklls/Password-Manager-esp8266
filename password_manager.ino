#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

const char* ap_ssid = "Thai's Pwd";
const char* ap_password = "12345679";

ESP8266WebServer server(80);

struct Account {
  char service[32];
  char username[64];
  char password[64];
  bool active;
};

Account accounts[40];
int accountCount = 0;

const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Password Manager ESP8266</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .container {
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            margin-bottom: 20px;
        }
        h1 {
            color: #333;
            text-align: center;
        }
        .form-group {
            margin-bottom: 15px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
        }
        input[type="text"], input[type="password"] {
            width: 100%;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 5px;
            box-sizing: border-box;
        }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            margin-right: 10px;
        }
        button:hover {
            background-color: #45a049;
        }
        .delete-btn {
            background-color: #f44336;
        }
        .delete-btn:hover {
            background-color: #da190b;
        }
        .account-item {
            border: 1px solid #ddd;
            padding: 15px;
            margin-bottom: 15px;
            border-radius: 5px;
            background-color: #f9f9f9;
        }
        .account-header {
            font-weight: bold;
            color: #333;
            margin-bottom: 10px;
            font-size: 18px;
        }
        .account-details {
            display: none;
            margin-top: 15px;
        }
        .individual-account {
            border: 1px solid #ccc;
            margin: 10px 0;
            padding: 10px;
            border-radius: 5px;
            background-color: #fff;
        }
        .show-details {
            background-color: #2196F3;
        }
        .show-details:hover {
            background-color: #0b7dda;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Password Manager of Thai handsome</h1>
        <h3>Add new account</h3>
        <form id="addForm">
            <div class="form-group">
                <label for="service">Service:</label>
                <input type="text" id="service" name="service" placeholder="Ex: Facebook, Gmail, Instagram..." required>
            </div>
            <div class="form-group">
                <label for="username">Username:</label>
                <input type="text" id="username" name="username" placeholder="Email or username" required>
            </div>
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="password" id="password" name="password" placeholder="Fill password" required>
            </div>
            <button type="submit">Add account</button>
        </form>
    </div>

    <div class="container">
        <h3>List accounts</h3>
        <div id="accountList">
            <p>Loading...</p>
        </div>
        <button onclick="loadAccounts()">Refresh</button>
    </div>

    <script>
        document.getElementById('addForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const formData = new FormData();
            formData.append('service', document.getElementById('service').value);
            formData.append('username', document.getElementById('username').value);
            formData.append('password', document.getElementById('password').value);
            
            fetch('/add', {
                method: 'POST',
                body: formData
            })
            .then(response => response.text())
            .then(data => {
                if (data === 'OK') {
                    alert('This account added success!');
                    document.getElementById('addForm').reset();
                    loadAccounts();
                } else {
                    alert('Have a error :< !');
                }
            });
        });

        function loadAccounts() {
            fetch('/list')
            .then(response => response.json())
            .then(data => {
                const accountList = document.getElementById('accountList');
                if (data.length === 0) {
                    accountList.innerHTML = '<p>List empty</p>';
                    return;
                }
                
                // Nhóm tài khoản theo dịch vụ
                const groupedAccounts = {};
                data.forEach((account, originalIndex) => {
                    const service = account.service.toLowerCase();
                    if (!groupedAccounts[service]) {
                        groupedAccounts[service] = {
                            serviceName: account.service,
                            accounts: []
                        };
                    }
                    groupedAccounts[service].accounts.push({
                        ...account,
                        originalIndex: originalIndex
                    });
                });
                
                let html = '';
                let groupIndex = 0;
                
                for (const serviceKey in groupedAccounts) {
                    const group = groupedAccounts[serviceKey];
                    html += '<div class="account-item">';
                    html += '<div class="account-header">Service: ' + group.serviceName + ' (' + group.accounts.length + ' accounts)</div>';
                    html += '<button class="show-details" onclick="toggleDetails(' + groupIndex + ')">more information</button>';
                    html += '<div id="details-' + groupIndex + '" class="account-details">';
                    
                    // Hiển thị tất cả tài khoản trong nhóm
                    group.accounts.forEach((account, accountIndex) => {
                        html += '<div style="border: 1px solid #ccc; margin: 10px 0; padding: 10px; border-radius: 5px; background-color: #fff;">';
                        html += '<p><strong>Account ' + (accountIndex + 1) + ':</strong></p>';
                        html += '<p><strong>Username:</strong> ' + account.username + '</p>';
                        html += '<p><strong>MPassword:</strong> ' + account.password + '</p>';
                        html += '<button class="delete-btn" onclick="deleteAccount(' + account.originalIndex + ')" style="margin-top: 5px;">Delete this account</button>';
                        html += '</div>';
                    });
                    
                    html += '</div></div>';
                    groupIndex++;
                }
                
                accountList.innerHTML = html;
            });
        }

        function toggleDetails(index) {
            const details = document.getElementById('details-' + index);
            if (details.style.display === 'none' || details.style.display === '') {
                details.style.display = 'block';
            } else {
                details.style.display = 'none';
            }
        }

        function deleteAccount(index) {
            if (confirm('Are u sure delete this acc?')) {
                fetch('/delete?index=' + index)
                .then(response => response.text())
                .then(data => {
                    if (data === 'OK') {
                        alert('Deleted!');
                        loadAccounts();
                    } else {
                        alert('Have a error!');
                    }
                });
            }
        }

        window.onload = function() {
            loadAccounts();
        };
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  delay(1000);
  

  EEPROM.begin(512);
  
 
  loadAccountsFromEEPROM();
  
 
  Serial.println("Khoi tao Access Point...");
  WiFi.mode(WIFI_AP);  
  IPAddress local_IP(192, 168, 10, 1);      
  IPAddress gateway(192, 168, 10, 1);       
  IPAddress subnet(255, 255, 255, 0);       
  WiFi.softAPConfig(local_IP, gateway, subnet);
 
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
 
  server.on("/", HTTP_GET, handleRoot);
  server.on("/add", HTTP_POST, handleAddAccount);
  server.on("/list", HTTP_GET, handleListAccounts);
  server.on("/delete", HTTP_GET, handleDeleteAccount);
  
  server.begin();
  
  // Serial.println("=================================");
  // Serial.println("ESP8266 Password Manager Ready!");
  // Serial.println("=================================");
  // Serial.print("WiFi Name: ");
  // Serial.println(ap_ssid);
  // Serial.print("Password: ");
  // Serial.println(ap_password);
  // Serial.print("Web Access: http://");
  // Serial.println(IP);
  // Serial.println("=================================");
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  server.send_P(200, "text/html", htmlPage);
}

void handleAddAccount() {
  if (accountCount >= 20) {
    server.send(400, "text/plain", "ERROR: Đã đạt giới hạn tài khoản");
    return;
  }
  
  String service = server.arg("service");
  String username = server.arg("username");
  String password = server.arg("password");
  
  if (service.length() > 0 && username.length() > 0 && password.length() > 0) {
    strncpy(accounts[accountCount].service, service.c_str(), 31);
    strncpy(accounts[accountCount].username, username.c_str(), 63);
    strncpy(accounts[accountCount].password, password.c_str(), 63);
    accounts[accountCount].active = true;
    
    accountCount++;
    
    saveAccountsToEEPROM();
    
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "ERROR: Thiếu thông tin");
  }
}

void handleListAccounts() {
  DynamicJsonDocument doc(4096);
  JsonArray array = doc.to<JsonArray>();
  
  for (int i = 0; i < accountCount; i++) {
    if (accounts[i].active) {
      JsonObject account = array.createNestedObject();
      account["service"] = accounts[i].service;
      account["username"] = accounts[i].username;
      account["password"] = accounts[i].password;
    }
  }
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleDeleteAccount() {
  int index = server.arg("index").toInt();
  
  if (index >= 0 && index < accountCount) {
    for (int i = index; i < accountCount - 1; i++) {
      accounts[i] = accounts[i + 1];
    }
    accountCount--;
    
    saveAccountsToEEPROM();
    
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "ERROR: Index không hợp lệ");
  }
}

void saveAccountsToEEPROM() {
  int addr = 0;
  
  EEPROM.write(addr, accountCount);
  addr++;
  
  for (int i = 0; i < accountCount; i++) {
    EEPROM.put(addr, accounts[i]);
    addr += sizeof(Account);
  }
  
  EEPROM.commit();
}

void loadAccountsFromEEPROM() {
  int addr = 0;
  
  accountCount = EEPROM.read(addr);
  addr++;
  
  if (accountCount > 40 || accountCount < 0) {
    accountCount = 0;
    return;
  }
  
  for (int i = 0; i < accountCount; i++) {
    EEPROM.get(addr, accounts[i]);
    addr += sizeof(Account);
  }
}