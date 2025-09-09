from PyQt5.QtWidgets import QWidget, QVBoxLayout, QLabel, QLineEdit, QPushButton, QMessageBox

class LoginWidget(QWidget):
    def __init__(self, api_client, on_login_success, parent=None):
        super().__init__(parent)
        self.api_client = api_client
        self.on_login_success = on_login_success
        self.init_ui()

    def init_ui(self):
        layout = QVBoxLayout()
        self.user_label = QLabel("Username:")
        self.user_edit = QLineEdit()
        self.pass_label = QLabel("Password:")
        self.pass_edit = QLineEdit()
        self.pass_edit.setEchoMode(QLineEdit.Password)
        self.login_btn = QPushButton("Login")
        self.login_btn.clicked.connect(self.try_login)
        layout.addWidget(self.user_label)
        layout.addWidget(self.user_edit)
        layout.addWidget(self.pass_label)
        layout.addWidget(self.pass_edit)
        layout.addWidget(self.login_btn)
        self.setLayout(layout)

    def try_login(self):
        username = self.user_edit.text()
        password = self.pass_edit.text()
        ok, token = self.api_client.login(username, password)
        if ok:
            self.on_login_success()
        else:
            QMessageBox.warning(self, "Login Failed", "Invalid username or password.")
