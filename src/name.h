/**
 *  Name.h
 * 
 *  Class that can be wrapped around a domain name (record owner)
 *  and that canonicalizes this name for comparison.
 * 
 *  This follows the algorithm described in RFC 4034. For more info,
 *  see https://tools.ietf.org/html/rfc4034#section-6
 * 
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2020 Copernica BV
 */

/**
 *  Include guard
 */
#pragma once

/**
 *  Dependencies
 */
#include <iostream>
#include "canonicalizer.h"

/**
 *  Begin of namespace
 */
namespace DNS {

/**
 *  Class definition
 */
class Name
{
private:
    /**
     *  Helper class holds a pointer and the size of a label
     */
    class Label 
    {
    private:
        /**
         *  The actual label (watch out, not null-terminated)
         *  @var const char 
         */
        const char *_label;
        
        /**
         *  Size of the label
         *  @var size_t
         */
        size_t _size;
    
    public:
        /**
         *  Constructor that extracts the first label
         *  @param  domain      one domain, possibly containing multiple labels  
         *  @throws std::runtime_error
         */
        Label(const char *domain) : _label(domain)
        {
            // locate the first dot
            const char *dot = strchr(_label, '.');
            
            // if there is no dot, this must have been the last label, 
            // otherwise we use the size up to the dot
            _size = dot == nullptr ? strlen(_label) : dot - domain;
            
            // we do expect _something_
            if (_size == 0) throw std::runtime_error("no label found");
            
            // label should not exceed max size
            if (_size > 63) throw std::runtime_error("label too long");
        }
        
        /**
         *  Destructor
         */
        virtual ~Label() = default;
        
        /**
         *  Size of the label
         *  @return size_t
         */
        size_t size() const
        {
            return _size;
        }
        
        /**
         *  Compare with a different label
         *  @param  that
         *  @return int
         */
        int compare(const Label &that) const
        {
            // compare the overlapping parts
            auto result = strncasecmp(_label, that._label, std::min(_size, that._size));
            
            // if we found a difference already, we found the smaller one
            if (result != 0) return result;
            
            // both names have a common prefix, the short label is the first one
            return _size - that._size;
        }

        /**
         *  Write the name to a canonical form
         *  @param  output      output object
         *  @return bool
         */
        bool canonicalize(Canonicalizer &output) const
        {
            // write the label size
            return output.add8(_size) && output.add((const unsigned char *)_label, _size);
        }

        /**
         *  Helper function to write the name to a stream
         *  @param  stream      stream to write to
         *  @param  sname       the label to write
         *  @return std::stream
         */
        friend std::ostream &operator<<(std::ostream &stream, const Label &label)
        {
            // write just the label
            // @todo convert to lowercase
            return stream.write(label._label, label._size);
        }
    };
    
    /**
     *  The name is constructed of a number of names
     *  @var std::vector
     */
    std::vector<Label> _labels;
    
    /**
     *  Get access to a label counted from the back (thus back(0) is the last label)
     *  You must ensure that you pass in a parameter that is not out of bounds
     *  @param  index
     *  @return Label
     */
    const Label &back(size_t index) const
    {
        // expose the requested label
        return _labels[_labels.size() - 1 - index];
    }
    

public:
    /**
     *  Constructor
     *  The name that you pass to this function must stay in scope during
     *  the lifetime of this object
     *  @param  name        null terminated string with a domain name
     *  @throws std::runtime_error
     */
    Name(const char *name)
    {
        // keep looping until we have parsed everything
        while (name[0])
        {
            // add a label (could throw)
            _labels.emplace_back(name);
            
            // prepare for next iteration
            name += _labels.back().size();
            
            // if we have ended up in a dot we proceed
            if (name[0] != '.') break;
            
            // pass the dot before we proceed with the next label
            name += 1;
        }
    }
    
    /**
     *  Destructor
     */
    virtual ~Name() = default;
    
    /**
     *  Number of labels
     *  @return size_t
     */
    size_t labels() const
    {
        // expose the number of labels
        return _labels.size();
    }
    
    /**
     *  Compare with a different name, ordering them according to the
     *  rules of RFC 4034 section 6,1
     *  @param  that        other name
     *  @return bool
     */
    bool operator<(const Name &that) const
    {
        // start comparing labels
        for (size_t i = 0; i < labels(); ++i)
        {
            // if the other object no longer has a label for this position, the other object comes first
            if (i >= that.labels()) return false;
            
            // compare the labels
            auto result = back(i).compare(that.back(i));
            
            // if the labels were equal we proceed with the next label
            if (result == 0) continue;
            
            // we're ready
            return result < 0;
        }
        
        // the current object ran out of labels, which means that it is small provided
        // the other object still has a label left
        return that.labels() > labels();
    }
    
    /**
     *  Compare for equality
     *  @param  that
     *  @return bool
     */
    bool operator==(const Name &that) const
    {
        // number of labels must match
        if (labels() != that.labels()) return false;
        
        // compare each and every label
        for (size_t i = 0; i < labels(); ++i)
        {
            // compare the labels
            if (_labels[i].compare(that._labels[i]) != 0) return false;
        }
        
        // we have a match
        return true;
    }

    /**
     *  Compare for equality
     *  @param  that
     *  @return bool
     */
    bool operator!=(const Name &that) const
    {
        // this is the opposite of ==
        return !operator==(that);
    }
        
    /**
     *  Write the name to a canonical form
     *  @todo support for wildcards / label-counter
     *  @param  output      output object
     *  @return bool
     */
    bool canonicalize(Canonicalizer &output) const
    {
        // write all labels
        for (const auto &label : _labels) 
        {
            // canonicalize the label
            if (!label.canonicalize(output)) return false;
        }
        
        // write null byte to mark the end
        return output.add8(0);
    }
    
    
    /**
     *  Helper function to write the name to a stream
     *  @param  stream      stream to write to
     *  @param  sname        the name to write
     *  @return std::stream
     */
    friend std::ostream &operator<<(std::ostream &stream, const Name &name)
    {
        // if there are no labels at all
        if (name.labels() == 0) return stream << '.';
        
        // write all labels
        for (const auto &label : name._labels) stream << label << '.';
        
        // expose the stream
        return stream;
    }
};

/**
 *  End of namespace
 */
}
