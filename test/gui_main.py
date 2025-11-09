import sys
from PyQt5.QtWidgets import QApplication, QStackedWidget
from api_client import ApiClient
from login_widget import LoginWidget
from main_widget import MainWidget
from constants import GUI_VERSION

BASE_URL = "http://192.168.1.198/"
BASE_IP = "192.168.1.198"

def main():
    app = QApplication(sys.argv)
    api_client = ApiClient(BASE_IP) # Pass BASE_IP instead of BASE_URL
    stack = QStackedWidget()

    def on_reconnect_requested():
        # Clear the current main_widget and go back to login
        while stack.count() > 1:
            widget = stack.widget(1)
            stack.removeWidget(widget)
            widget.deleteLater()
        stack.setCurrentWidget(login_widget)

    def on_login_success(new_ip): # Modified to accept new_ip
        api_client.base_ip = new_ip                                                                                                                                          
        api_client.base_url = f"http://{new_ip}/" 
        main_widget = MainWidget(api_client, new_ip) # Pass new_ip to MainWidget
        main_widget.reconnect_requested.connect(on_reconnect_requested)
        stack.addWidget(main_widget)
        stack.setCurrentWidget(main_widget)

    login_widget = LoginWidget(api_client, on_login_success, BASE_IP) # Pass BASE_IP as initial_ip
    stack.addWidget(login_widget)
    stack.setCurrentWidget(login_widget)
    stack.setWindowTitle("GG-Prodino Tester GUI")
    stack.resize(600, 600)
    stack.show()
    sys.exit(app.exec_())

if __name__ == "__main__":
    main()
