import os.path
import csv
import requests
from bs4 import BeautifulSoup

def get_info(url):
    """Fetch and parse information from a Wikipedia URL.

    Args:
        url (str): The URL of the Wikipedia page.

    Returns:
        tuple: A tuple containing the title and the first non-empty paragraph of the Wikipedia page.
               Returns False if the URL is invalid or the page could not be processed.
    """
    if not "wikipedia.org" in url:  # Check if the URL is a Wikipedia link
        return False
    try:
        response = requests.get(url)  # Send a GET request to the URL
        if response.status_code != 200:  # Check if the request was successful
            raise Exception()
        content = BeautifulSoup(response.text, "html.parser")  # Parse the HTML content of the page
        title = content.select_one(".firstHeading").text  # Extract the page title
        paragraphs = content.select("div.mw-parser-output > p")  # Extract all paragraphs
        paragraph = None
        for p in paragraphs:
            if p.text.strip() == "":  # Skip empty paragraphs
                continue
            paragraph = p.text.strip()  # Get the first non-empty paragraph
            break
        if paragraph is None:  # Check if a valid paragraph was found
            return False
        return title, paragraph  # Return the title and paragraph
    except:  # Handle any exceptions
        return False

file = None
if not os.path.isfile("Wikipedia_Content.csv"):  # Check if the CSV file exists
    file = open("Wikipedia_Content.csv", "w", newline="", encoding="utf-8")  # Create a new CSV file if it does not exist
    file.write("title,description\n")  # Write the header row
    file.flush()
else:
    file = open("Wikipedia_Content.csv", "a", newline="", encoding="utf-8")  # Open the CSV file in append mode
writer = csv.writer(file)  # Create a CSV writer object

while True:
    print("Wikipedia URL: ", end="")
    url = input()  # Get the Wikipedia URL from user input
    res = get_info(url)  # Fetch information from the URL
    if not res:  # Check if the URL is invalid or the information could not be fetched
        print("Invalid url")
        continue
    title, description = res  # Extract the title and description
    print(f"Scraped Output:\nTitle: {title}\nDescription: {description}")  # Print the scraped information
    writer.writerow([title, description])  # Write the information to the CSV file
    file.flush()  # Flush the file buffer to ensure data is written to the file
    break  # Exit the loop
