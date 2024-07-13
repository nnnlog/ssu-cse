import sqlite3

# Connect to the SQLite database 'game.db', allowing connections from multiple threads
con = sqlite3.connect("game.db", check_same_thread=False)

# Create the SCORE table if it doesn't exist, with columns for user ID and score
con.execute("""
CREATE TABLE IF NOT EXISTS SCORE (id VARCHAR(255) PRIMARY KEY, score INTEGER)
""")

# Create the LOG table if it doesn't exist, with columns for user ID, log message, and a primary key for log entries
con.execute("""
CREATE TABLE IF NOT EXISTS LOG (id VARCHAR(255), log VARCHAR(255), logid INTEGER PRIMARY KEY AUTOINCREMENT)
""")

def get_user_score(user):
    """Retrieve the score for a given user. If the user does not exist in the SCORE table, add them with a score of 0."""
    value = con.execute(f"SELECT score FROM SCORE WHERE id = '{user}'").fetchone()
    if value is None:  # If user does not exist in the SCORE table
        con.execute(f"INSERT INTO SCORE (id, score) VALUES ('{user}', 0)")  # Insert the user with score 0
        con.commit()  # Commit the transaction
        return 0  # Return a score of 0
    return value[0]  # Return the user's score

def increment_score(user):
    """Increment the score for a given user by 1."""
    con.execute(f"UPDATE SCORE SET score = score + 1 WHERE id = '{user}'")  # Update the score by incrementing it by 1
    con.commit()  # Commit the transaction

def append_log(user, log):
    """Append a log entry for a given user."""
    con.execute(f"INSERT INTO LOG (id, log) VALUES (?, ?)", (user, log))  # Insert a log entry with the user ID and log message
    con.commit()  # Commit the transaction
