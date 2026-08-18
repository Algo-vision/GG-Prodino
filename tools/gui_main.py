import sys
from PyQt5.QtWidgets import QApplication, QStackedWidget
from api_client import ApiClient
from login_widget import LoginWidget
from main_widget import MainWidget
from constants import GUI_VERSION

BASE_URL = "http://192.168.1.198/"
BASE_IP = "192.168.1.198"

# Support for multi-client staggered polling
# Usage: python gui_main.py [offset_ms]
# Example: 
#   Computer 1: python gui_main.py 0
#   Computer 2: python gui_main.py 666
#   Computer 3: python gui_main.py 1332
CLIENT_OFFSET_MS = int(sys.argv[1]) if len(sys.argv) > 1 else 0


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
        main_widget = MainWidget(api_client, new_ip, client_offset_ms=CLIENT_OFFSET_MS) # Pass offset
        main_widget.reconnect_requested.connect(on_reconnect_requested)
        stack.addWidget(main_widget)
        stack.setCurrentWidget(main_widget)

    login_widget = LoginWidget(api_client, on_login_success, BASE_IP) # Pass BASE_IP as initial_ip
    stack.addWidget(login_widget)
    stack.setCurrentWidget(login_widget)
    
    # Update window title to show client offset (helps identify multiple instances)
    title = "GG-GRK Tester GUI"
    if CLIENT_OFFSET_MS > 0:
        title += f" (Client Offset: {CLIENT_OFFSET_MS}ms)"
    stack.setWindowTitle(title)

    # Set minimum size but allow resizing to fit any screen
    stack.setMinimumSize(700, 600)
    # Start maximized to fit the screen
    stack.showMaximized()
    sys.exit(app.exec_())

if __name__ == "__main__":
    main()
