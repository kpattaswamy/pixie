import pymongo
import os
import time

#print("sleep done")
#time.sleep(60)
print("Starting MongoDB client")
#time.sleep(60)

client = pymongo.MongoClient("mongodb://localhost:27017/?timeoutMS=2000")

db = client["carDB"]
collection = db["car"]

# Insert document
print("Inserting document")
car = { "name": "Honda" }
resp = collection.insert_one(car)
print(resp)


# Find document
print("Finding the inserted document")
resp = collection.find_one()
print(resp)


# Update document
print("Updating the document")
newCar = { "$set": {"name": "Toyota"} }
resp = collection.update_one(car, newCar)
print(resp)

# Find updated document
print("Finding the updated document")
resp = collection.find_one()
print(resp)

# Delete document
print("Delete the document")
resp = collection.delete_one({"name": "Toyota"})
print(resp)
