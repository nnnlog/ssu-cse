import datetime
import random
import uuid

from flask import session, Flask, redirect, url_for, request, render_template

import game_db

app = Flask(__name__)

app.debug = True  # Enable debug mode for the Flask application
app.config['DEBUG'] = True  # Ensure the debug mode is set to True

# Set the secret key to some random bytes. Keep this really secret!
app.secret_key = b'asdfadfsdffsdf'  # Secret key for session management

def reset_game():
    """Reset the game by initializing the number of tries and setting a new random number."""
    session["try"] = 0
    session["number"] = random.randint(1, 10)

@app.route('/')
def index():
    """Home route that checks if the user is logged in and redirects accordingly."""
    if not "id" in session:  # Check if user is logged in
        return redirect(url_for('login'))  # Redirect to login if not logged in
    return redirect(url_for('game'))  # Redirect to game if logged in

@app.route('/game', methods=['GET', 'POST'])
def game():
    """Route for the game logic where users can guess the number."""
    if not "id" in session:  # Check if user is logged in
        return redirect(url_for('login'))  # Redirect to login if not logged in
    msg = None  # Initialize message
    score = game_db.get_user_score(session["id"])  # Get user's score from database
    if request.method == 'POST':
        if session["csrf"] != request.form['csrf']:  # Check CSRF token for security
            return redirect(url_for("game"))  # Redirect if CSRF token is invalid
        value = request.form['value']  # Get the value submitted by user
        try:
            value = int(value)  # Try to convert the value to an integer
            if value < 1 or value > 10:  # Check if the number is within the valid range
                msg = "Number must be between 1 and 10"
        except:
            msg = "Not number"  # Handle invalid input that is not a number
        if msg is None:
            if session["number"] == value:  # Check if the guessed number is correct
                msg = "Congratulations, you did it, start new round"
                reset_game()  # Reset game on correct guess
                score += 1  # Increment score
                game_db.increment_score(session["id"])  # Update score in the database
            else:
                msg = "Hint: You guessed too small!" if session["number"] > value else "Hint: You guessed too high!"
                session["try"] += 1  # Increment the number of tries
                if session["try"] >= 5:  # Check if user has exceeded the number of tries
                    msg = "Sorry, you've used all your attempts, we start new round!"
                    reset_game()  # Reset game if tries are exhausted
    return render_template("game.html", msg=msg, score=score, csrf=session["csrf"])  # Render the game template with message, score, and CSRF token

@app.route('/login', methods=['GET', 'POST'])
def login():
    """Route for handling user login."""
    err_msg = None  # Initialize error message
    if request.method == 'POST':
        id = request.form['id']  # Get user ID from form
        pw = request.form['pw']  # Get password from form
        table = {"asdf": "asdf"}  # Hardcoded user table (for demonstration purposes)
        if id in table and pw == table[id]:  # Check if credentials are correct
            session['id'] = request.form['id']  # Set user ID in session
            session["csrf"] = str(uuid.uuid4())  # Generate and set CSRF token
            game_db.append_log(session["id"], "[%s] %s login" % (str(datetime.datetime.now()), session["id"]))  # Log the login event
            reset_game()  # Reset the game state
            return redirect(url_for('index'))  # Redirect to index after login
        else:
            err_msg = "Wrong username or password"  # Set error message for invalid login
    return render_template("login.html", msg=err_msg)  # Render login template with error message

@app.route('/logout')
def logout():
    """Route for handling user logout."""
    if "id" in session:  # Check if user is logged in
        game_db.append_log(session["id"], "[%s] %s logout" % (str(datetime.datetime.now()), session["id"]))  # Log the logout event
    session.clear()  # Clear the session
    return redirect(url_for('index'))  # Redirect to index after logout
