from telegram.ext import Updater, CommandHandler
import random

TOKEN = "7290998230:AAGi2WAFE0QgAWh-FmmyhAU5sRyGVbkFMbc"


user_otps = {}

def generate_otp():
    """Tạo OTP 6 chữ số ngẫu nhiên."""
    return random.randint(100000, 999999)

def start(update, context):
    update.message.reply_text("Chào mừng bạn đến với hệ thống OTP!\nGõ /otp để nhận OTP.")

def otp(update, context):
    user_id = update.message.from_user.id
    otp_code = generate_otp()
    user_otps[user_id] = otp_code
    update.message.reply_text(f"OTP của bạn là: {otp_code}")

def verify(update, context):
    user_id = update.message.from_user.id
    try:
        entered_otp = int(''.join(context.args))
    except Exception:
        update.message.reply_text("Cú pháp: /verify <OTP>")
        return

    if user_id in user_otps and user_otps[user_id] == entered_otp:
        update.message.reply_text("Xác thực OTP thành công!")
        del user_otps[user_id]
    else:
        update.message.reply_text("OTP không đúng, vui lòng thử lại.")

def main():
    updater = Updater(TOKEN, use_context=True)
    dp = updater.dispatcher

    dp.add_handler(CommandHandler("start", start))
    dp.add_handler(CommandHandler("otp", otp))
    dp.add_handler(CommandHandler("verify", verify))

    updater.start_polling()
    print("Bot dang chay...")  
    updater.idle()

if __name__ == '__main__':
    main()
