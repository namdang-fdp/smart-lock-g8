from flask import Flask, render_template, request, redirect, url_for, session, flash
from replit import db
from datetime import datetime
from flask import jsonify
from flask import render_template_string

app = Flask(__name__)
app.secret_key = "your_secret_key"

# Nếu trong DB chưa có người dùng, khởi tạo dữ liệu mẫu
db["users"] = {
    "admin": {
        "password": "123456",
        "role": "admin"
    },
    "user1": {
        "password": "pass123",
        "role": "user"
    }
}

if "sensor_data" not in db.keys():
  db["sensor_data"] = [{
      "id": 1,
      "value": "45",
      "timestamp": "2025-03-20 05:30:00"
  }, {
      "id": 2,
      "value": "62",
      "timestamp": "2025-03-20 05:45:00"
  }, {
      "id": 3,
      "value": "58",
      "timestamp": "2025-03-20 06:00:00"
  }]


@app.route("/login", methods=["GET", "POST"])
def login():
  if request.method == "POST":
    username = request.form["username"]
    password = request.form["password"]
    print(username)
    print(password)
    users = db["users"]
    if username in users and users[username]["password"] == password:
      session["username"] = username
      session["role"] = users[username]["role"]
      if users[username]["role"] == "admin":
        return redirect(url_for("admin"))
      else:
        return redirect(url_for("dashboard"))
    else:
      flash("Tên đăng nhập hoặc mật khẩu không đúng!", "error")
  return render_template("login.html")



@app.route("/")
def home():
  return redirect(url_for("login"))


@app.route("/dashboard")
def dashboard():
  if "username" not in session:
    return redirect(url_for("login"))

  sensor_data = db["sensor_data"]

  return render_template("dashboard.html",
                         username=session["username"],
                         role=session["role"],
                         theme=session.get("theme", "light"),
                         sensor_data=sensor_data)


@app.route("/admin")
def admin():
  if "username" not in session or session.get("role") != "admin":
    flash("Bạn không có quyền truy cập trang này.", "error")
    return redirect(url_for("login"))
  users = db["users"]
  return render_template("admin.html",
                         username=session["username"],
                         users=users)


@app.route("/admin/edit/<username>", methods=["GET", "POST"])
def edit_user(username):
  # Chỉ cho phép admin truy cập
  if "username" not in session or session.get("role") != "admin":
    flash("Bạn không có quyền truy cập trang này.", "error")
    return redirect(url_for("dashboard"))

  users = db["users"]
  # Kiểm tra user tồn tại
  if username not in users:
    flash("User không tồn tại.", "error")
    return redirect(url_for("admin"))

  if request.method == "POST":
    new_password = request.form["password"]
    new_role = request.form["role"]
    users[username] = {"password": new_password, "role": new_role}
    db["users"] = users
    flash("User được cập nhật thành công!", "success")
    return redirect(url_for("admin"))

  # GET: render trang sửa user với thông tin hiện tại
  return render_template("edit_user.html",
                         username=username,
                         user=users[username])


@app.route("/admin/add", methods=["GET", "POST"])
def add_user():
  if "username" not in session or session.get("role") != "admin":
    flash("Bạn không có quyền truy cập.", "error")
    return redirect(url_for("dashboard"))
  if request.method == "POST":
    new_username = request.form["username"]
    new_password = request.form["password"]
    new_role = request.form["role"]
    users = db["users"]
    if new_username in users:
      flash("User đã tồn tại.", "error")
    else:
      users[new_username] = {"password": new_password, "role": new_role}
      db["users"] = users
      flash("User được thêm thành công!", "success")
      return redirect(url_for("admin"))
  return render_template("add_user.html")


@app.route("/admin/delete/<username>")
def delete_user(username):
  # Kiểm tra đăng nhập và quyền admin
  if "username" not in session or session.get("role") != "admin":
    flash("Bạn không có quyền truy cập trang này.", "error")
    return redirect(url_for("dashboard"))

  users = db["users"]
  # Kiểm tra xem user có tồn tại hay không
  if username in users:
    del users[username]
    db["users"] = users
    flash("User đã được xóa thành công!", "success")
  else:
    flash("User không tồn tại.", "error")

  return redirect(url_for("admin"))


# Endpoint nhận dữ liệu từ mạch IoT (ESP32)
@app.route("/iot/data", methods=["GET"])
def iot_data():
  # Lấy giá trị cảm biến ánh sáng từ tham số 'light'
  light_value = request.args.get("light")
  if light_value is not None:
    sensor_data = db["sensor_data"]
    new_entry = {
        "id": len(sensor_data) + 1,
        "value": light_value,
        "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    }
    sensor_data.append(new_entry)
    db["sensor_data"] = sensor_data
    return "Data received", 200
  else:
    return "No data received", 400


@app.route("/api/sensor_data")
def api_sensor_data():
  sensor_data = db["sensor_data"]  # ObservedList
  sensor_data_list = []

  # Chuyển từng ObservedDict thành dict thường
  for item in sensor_data:
    normal_item = {
        "id": item["id"],
        "value": item["value"],
        "timestamp": item["timestamp"]
    }
    sensor_data_list.append(normal_item)

  # Bây giờ sensor_data_list là list[dict], jsonify được
  return jsonify(sensor_data_list)


@app.route("/toggle_theme")
def toggle_theme():
  """Chuyển giữa light/dark."""
  current_theme = session.get("theme", "light")
  new_theme = "dark" if current_theme == "light" else "light"
  session["theme"] = new_theme
  return redirect(url_for("dashboard"))


# Đăng xuất
@app.route("/logout")
def logout():
  session.pop("username", None)
  session.pop("role", None)
  return redirect(url_for("login"))


if __name__ == "__main__":
  app.run(host="0.0.0.0", port=8080, debug=True)
