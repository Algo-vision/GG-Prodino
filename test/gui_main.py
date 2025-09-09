import sys
from PyQt5.QtWidgets import QApplication, QStackedWidget
from api_client import ApiClient
from login_widget import LoginWidget
from main_widget import MainWidget

BASE_URL = "http://192.168.1.198/"
BASE_IP = "192.168.1.198"

def main():
    app = QApplication(sys.argv)
    api_client = ApiClient(BASE_URL)
    stack = QStackedWidget()

    def on_login_success():
        main_widget = MainWidget(api_client, BASE_IP)
        stack.addWidget(main_widget)
        stack.setCurrentWidget(main_widget)

    login_widget = LoginWidget(api_client, on_login_success)
    stack.addWidget(login_widget)
    stack.setCurrentWidget(login_widget)
    stack.setWindowTitle("GG-Prodino Tester GUI")
    stack.resize(600, 600)
    stack.show()
    sys.exit(app.exec_())

if __name__ == "__main__":
    main()
