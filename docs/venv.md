# how to handle venv on school computers

so i do not have sudo rights

i can install venv without pip
```
python3 -m venv --without-pip venv
```

So this creates a venv directory without pip
I download get-pip.py and run it
```
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
python get-pip.py
pip install pytest
```

evenbtually disactivate the venv and activate it again
```
source venv/bin/activate
```

Now:
```
which pytest  # Should show `venv/bin/pytest`
```

Run make but better to start it outside the env first
