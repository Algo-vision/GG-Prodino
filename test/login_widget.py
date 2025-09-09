from PyQt5.QtWidgets import QWidget, QVBoxLayout, QLabel, QLineEdit, QPushButton, QMessageBox

class LoginWidget(QWidget):
    def __init__(self, api_client, on_login_success, parent=None):
        super().__init__(parent)
        self.api_client = api_client
        self.on_login_success = on_login_success
        self.init_ui()

    def init_ui(self):
        from PyQt5.QtWidgets import QFormLayout, QHBoxLayout, QSpacerItem, QSizePolicy
        outer_layout = QVBoxLayout()
        form_layout = QFormLayout()
        self.user_edit = QLineEdit()
        self.pass_edit = QLineEdit()
        self.pass_edit.setEchoMode(QLineEdit.Password)
        form_layout.addRow("Username:", self.user_edit)
        form_layout.addRow("Password:", self.pass_edit)
        self.login_btn = QPushButton("Login")
        self.login_btn.clicked.connect(self.try_login)
        btn_layout = QHBoxLayout()
        btn_layout.addStretch(1)
        btn_layout.addWidget(self.login_btn)
        btn_layout.addStretch(1)
        outer_layout.addStretch(2)
        outer_layout.addLayout(form_layout)
        outer_layout.addLayout(btn_layout)
        outer_layout.addStretch(3)
        self.setLayout(outer_layout)

    def try_login(self):
        username = self.user_edit.text()
        password = self.pass_edit.text()
        ok, token = self.api_client.login(username, password)
        if ok:
            self.on_login_success()
        else:
            QMessageBox.warning(self, "Login Failed", "Invalid username or password.")
