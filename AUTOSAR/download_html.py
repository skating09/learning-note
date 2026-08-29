import requests
from bs4 import BeautifulSoup

# Ask user to input the URL
url = input("Enter CSDN article URL: ")

# Fetch the webpage
response = requests.get(url, headers={"User-Agent": "Mozilla/5.0"})
response.encoding = "utf-8"

# Parse HTML
soup = BeautifulSoup(response.text, "html.parser")

# Find the main content area
content = soup.find("div", class_="blog-content")
if content is None:
    content = soup.find("div", class_="article_content")

# Save to local HTML file
with open("csdn_article.html", "w", encoding="utf-8") as f:
    f.write("<html><head><meta charset='utf-8'></head><body>")
    if content:
        f.write(str(content))
    else:
        f.write("Content not found")
    f.write("</body></html>")

print("Article saved as csdn_article.html")
