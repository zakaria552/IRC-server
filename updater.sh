while :
do
    git pull | grep 'Already up to date'
    if [ $? == "1" ]; then
        killall ircserv
    fi
    sleep 10
done

