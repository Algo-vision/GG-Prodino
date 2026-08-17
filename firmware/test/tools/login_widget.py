from PyQt5.QtWidgets import QWidget, QVBoxLayout, QLabel, QLineEdit, QPushButton, QMessageBox
from PyQt5 import QtCore # Import QtCore
from constants import GUI_VERSION # Import GUI_VERSION

class LoginWidget(QWidget):
    def __init__(self, api_client, on_login_success, initial_ip, parent=None):
        super().__init__(parent)
        self.api_client = api_client
        self.on_login_success = on_login_success
        self.initial_ip = initial_ip # Store initial IP
        self.init_ui()

    def init_ui(self):
        from PyQt5.QtWidgets import QFormLayout, QHBoxLayout, QSpacerItem, QSizePolicy
        outer_layout = QVBoxLayout()
        form_layout = QFormLayout()

        self.ip_edit = QLineEdit(self.initial_ip) # IP input field
        self.user_edit = QLineEdit()
        self.pass_edit = QLineEdit()
        self.pass_edit.setEchoMode(QLineEdit.Password)

        form_layout.addRow("Controller IP:", self.ip_edit) # Add IP field to layout
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

        # Display GUI Version
        version_label = QLabel(f"GUI Version: {GUI_VERSION}")
        version_label.setAlignment(QtCore.Qt.AlignCenter) # Align to center
        outer_layout.addWidget(version_label)

        self.setLayout(outer_layout)

    def try_login(self):
        controller_ip = self.ip_edit.text() # Get IP from input field
        username = self.user_edit.text()
        password = self.pass_edit.text()

        # Update api_client's base_ip and base_url before login attempt
        self.api_client.base_ip = controller_ip
        self.api_client.base_url = f"http://{controller_ip}/"

        ok, message = self.api_client.login(username, password)
        if ok:
            self.on_login_success(controller_ip) # Pass IP to success callback
        else:
            if message == "Connection Error" or message == "Connection Timeout":
                QMessageBox.critical(self, "Connection Error", "No communication with controller.")
            else:
                QMessageBox.warning(self, "Login Failed", "Invalid username or password.")
