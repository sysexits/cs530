sudo make modules -j8
sudo make modules_install
sudo make bzImage -j8
sudo make install
sudo update-grub
sudo update-grub2
