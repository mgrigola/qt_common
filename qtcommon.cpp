#include "QtCommon.h"

//returns true if item was changed. Else returns false
bool Add_To_ComboBox_History(QComboBox* pComboBox, QString stringToAdd)
{
    int existingPos = pComboBox->findText(stringToAdd);
    if (existingPos == -1)  //item not in list -> add it
    {
        pComboBox->addItem(stringToAdd);
        return true;
    }
    else if (existingPos != (pComboBox->count() - 1) ) //item not already at end of list -> move to end
    {
        pComboBox->removeItem(existingPos);
        pComboBox->addItem(stringToAdd);
        //pComboBox->setCurrentText( stringToAdd );  //not necessary?
        return true;
    }
    //else item is already at end of list -> do nothing

    return false;
}
