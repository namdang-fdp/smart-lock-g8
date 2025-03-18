from flask import Flask, render_template, request, redirect, url_for, session

app = Flask(__name__)
app.secret_key = "your_secret_key"  # Đổi thành chuỗi bí mật của bạn

# Trang chủ: hiển thị dữ liệu nếu đã đăng nhập, nếu chưa chuyển sang login
@app.route('/')
def index():
    if "user" in session:
        # Giả sử bạn có dữ liệu đo đạc được lưu trong biến data (bạn có thể truy vấn từ database)
        data = {"temperature": 25, "humidity": 60}  
        return f"Welcome, {session['user']}!<br>Data: {data}"
    else:
        return redirect(url_for("login"))

# Trang đăng nhập
@app.route('/login', methods=["GET", "POST"])
def login():
    if request.method == "POST":
        username = request.form["username"]
        password = request.form["password"]
        # Ở đây bạn có thể kiểm tra thông tin đăng nhập từ cơ sở dữ liệu hoặc một giá trị cứng
        if username == "admin" and password == "1234":
            session["user"] = username
            return redirect(url_for("index"))
        else:
            return "Invalid credentials, please try again."
    return '''
    <form method="post">
      Username: <input name="username"><br>
      Password: <input name="password" type="password"><br>
      <input type="submit" value="Login">
    </form>
    '''

if __name__ == "__main__":
    app.run(debug=True)
